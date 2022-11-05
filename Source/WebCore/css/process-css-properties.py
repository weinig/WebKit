#!/usr/bin/env python3
#
# Copyright (C) 2022 Apple Inc. All rights reserved.
#
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions
# are met:
# 1. Redistributions of source code must retain the above copyright
#    notice, this list of conditions and the following disclaimer.
# 2. Redistributions in binary form must reproduce the above copyright
#    notice, this list of conditions and the following disclaimer in the
#    documentation and/or other materials provided with the distribution.
#
# THIS SOFTWARE IS PROVIDED BY APPLE INC. AND ITS CONTRIBUTORS ``AS IS''
# AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
# THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
# PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL APPLE INC. OR ITS CONTRIBUTORS
# BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
# CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
# SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
# INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
# CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
# ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF
# THE POSSIBILITY OF SUCH DAMAGE.

import argparse
import functools
import itertools
import json
import os
import re
import subprocess
import sys
import textwrap


class ParsingContext:
    def __init__(self, *, defines_string, parsing_for_codegen, verbose):
        if defines_string:
            self.conditionals = frozenset(defines_string.split(' '))
        else:
            self.conditionals = frozenset()
        self.parsing_for_codegen = parsing_for_codegen
        self.verbose = verbose

    def is_enabled(self, *, conditional):
        if conditional[0] == '!':
            return conditional[1:] not in self.conditionals
        return conditional in self.conditionals

    def select_enabled_variant(self, variants, *, label):
        for variant in variants:
            if "enable-if" not in variant:
                raise Exception(f"Invalid conditional definition for '{label}'. No 'enable-if' property found.")

            if self.is_enabled(conditional=variant["enable-if"]):
                return variant

        raise Exception(f"Invalid conditional definition for '{label}'. No 'enable-if' property matched the active set.")


class Schema:
    class Entry:
        def __init__(self, key, *, allowed_types, default_value=None):
            self.key = key
            self.allowed_types = allowed_types
            self.default_value = default_value

    def __init__(self, *entries):
        self.entries = {entry.key: entry for entry in entries}

    def set_attributes_from_dictionary(self, dictionary, *, instance):
        for entry in self.entries.values():
            setattr(instance, entry.key.replace("-", "_"), dictionary.get(entry.key, entry.default_value))

    def validate_keys(self, dictionary, *, label):
        invalid_keys = list(filter(lambda key: key not in self.entries.keys(), dictionary.keys()))
        if len(invalid_keys) == 1:
            raise Exception(f"Invalid key for '{label}': {invalid_keys[0]}")
        if len(invalid_keys) > 1:
            raise Exception(f"Invalid keys for '{label}': {invalid_keys}")

    def validate_types(self, dictionary, *, label):
        for key, value in dictionary.items():
            if type(value) not in self.entries[key].allowed_types:
                raise Exception(f"Invalid type '{type(value)}' for key '{key}' in '{label}'. Expected type in set '{self.entries[key].allowed_types}'")

    def validate_dictionary(self, dictionary, *, label):
        self.validate_keys(dictionary, label=label)
        self.validate_types(dictionary, label=label)


class Status:
    schema = Schema(
        Schema.Entry("comment", allowed_types=[str]),
        Schema.Entry("enabled-by-default", allowed_types=[bool]),
        Schema.Entry("status", allowed_types=[str]),
    )

    def __init__(self, **dictionary):
        Status.schema.set_attributes_from_dictionary(dictionary, instance=self)

    def __str__(self):
        return f"Status {vars(self)}"

    def __repr__(self):
        return self.__str__()

    @staticmethod
    def from_json(parsing_context, key_path, json_value):
        if type(json_value) is str:
            return Status(status=json_value)

        assert(type(json_value) is dict)
        Status.schema.validate_dictionary(json_value, label=f"Status ({key_path}.status)")

        return Status(**json_value)


class Specification:
    schema = Schema(
        Schema.Entry("category", allowed_types=[str]),
        Schema.Entry("comment", allowed_types=[str]),
        Schema.Entry("description", allowed_types=[str]),
        Schema.Entry("documentation-url", allowed_types=[str]),
        Schema.Entry("keywords", allowed_types=[list], default_value=[]),
        Schema.Entry("non-canonical-url", allowed_types=[str]),
        Schema.Entry("obsolete-category", allowed_types=[str]),
        Schema.Entry("obsolete-url", allowed_types=[str]),
        Schema.Entry("url", allowed_types=[str]),
    )

    def __init__(self, **dictionary):
        Specification.schema.set_attributes_from_dictionary(dictionary, instance=self)

    def __str__(self):
        return f"Specification {vars(self)}"

    def __repr__(self):
        return self.__str__()

    @staticmethod
    def from_json(parsing_context, key_path, json_value):
        assert(type(json_value) is dict)
        Specification.schema.validate_dictionary(json_value, label=f"Specification ({key_path}.specification)")
        return Specification(**json_value)


class Value:
    schema = Schema(
        Schema.Entry("comment", allowed_types=[str]),
        Schema.Entry("enable-if", allowed_types=[str]),
        Schema.Entry("status", allowed_types=[str]),
        Schema.Entry("url", allowed_types=[str]),
        Schema.Entry("value", allowed_types=[str]),
    )

    def __init__(self, **dictionary):
        Value.schema.set_attributes_from_dictionary(dictionary, instance=self)

    def __str__(self):
        return f"Value {vars(self)}"

    def __repr__(self):
        return self.__str__()

    @staticmethod
    def from_json(parsing_context, key_path, json_value):
        if type(json_value) is str:
            return Value(value=json_value)

        assert(type(json_value) is dict)
        Value.schema.validate_dictionary(json_value, label=f"Value ({key_path}.values)")

        if "enable-if" in json_value and not parsing_context.is_enabled(conditional=json_value["enable-if"]):
            return None

        return Value(**json_value)


class LogicalPropertyGroup:
    schema = Schema(
        Schema.Entry("name", allowed_types=[str]),
        Schema.Entry("resolver", allowed_types=[str]),
    )

    logical_property_group_resolvers = {
        "logical": {
            # Order matches LogicalBoxAxis enum in Source/WebCore/platform/text/WritingMode.h.
            "axis": ["inline", "block"],
            # Order matches LogicalBoxSide enum in Source/WebCore/platform/text/WritingMode.h.
            "side": ["block-start", "inline-end", "block-end", "inline-start"],
            # Order matches LogicalBoxCorner enum in Source/WebCore/platform/text/WritingMode.h.
            "corner": ["start-start", "start-end", "end-start", "end-end"],
        },
        "physical": {
            # Order matches BoxAxis enum in Source/WebCore/platform/text/WritingMode.h.
            "axis": ["horizontal", "vertical"],
            # Order matches BoxSide enum in Source/WebCore/platform/text/WritingMode.h.
            "side": ["top", "right", "bottom", "left"],
            # Order matches BoxCorner enum in Source/WebCore/platform/text/WritingMode.h.
            "corner": ["top-left", "top-right", "bottom-right", "bottom-left"],
        },
    }

    def __init__(self, **dictionary):
        LogicalPropertyGroup.schema.set_attributes_from_dictionary(dictionary, instance=self)
        self._update_kind_and_logic()

    def __str__(self):
        return f"LogicalPropertyGroup {vars(self)}"

    def __repr__(self):
        return self.__str__()

    def _update_kind_and_logic(self):
        for current_logic, current_resolvers_for_logic in LogicalPropertyGroup.logical_property_group_resolvers.items():
            for current_kind, resolver_list in current_resolvers_for_logic.items():
                for current_resolver in resolver_list:
                    if current_resolver == self.resolver:
                        self.kind = current_kind
                        self.logic = current_logic
                        return
        raise Exception(f"Unrecognized resolver \"{self.resolver}\"")

    @staticmethod
    def from_json(parsing_context, key_path, json_value):
        assert(type(json_value) is dict)
        LogicalPropertyGroup.schema.validate_dictionary(json_value, label=f"LogicalPropertyGroup ({key_path}.logical-property-group)")
        return LogicalPropertyGroup(**json_value)

class CodeGenProperties:
    schema = Schema(
        Schema.Entry("aliases", allowed_types=[list], default_value=[]),
        Schema.Entry("auto-functions", allowed_types=[bool], default_value=False),
        Schema.Entry("color-property", allowed_types=[bool], default_value=False),
        Schema.Entry("comment", allowed_types=[str]),
        Schema.Entry("computable", allowed_types=[bool]),
        Schema.Entry("conditional-converter", allowed_types=[str]),
        Schema.Entry("converter", allowed_types=[str]),
        Schema.Entry("custom", allowed_types=[str]),
        Schema.Entry("descriptor-only", allowed_types=[bool], default_value=False),
        Schema.Entry("enable-if", allowed_types=[str]),
        Schema.Entry("fast-path-inherited", allowed_types=[bool], default_value=False),
        Schema.Entry("fill-layer-property", allowed_types=[bool], default_value=False),
        Schema.Entry("font-property", allowed_types=[bool], default_value=False),
        Schema.Entry("getter", allowed_types=[str]),
        Schema.Entry("high-priority", allowed_types=[bool], default_value=False),
        Schema.Entry("initial", allowed_types=[str]),
        Schema.Entry("internal-only", allowed_types=[bool], default_value=False),
        Schema.Entry("logical-property-group", allowed_types=[dict]),
        Schema.Entry("longhands", allowed_types=[list]),
        Schema.Entry("name-for-methods", allowed_types=[str]),
        Schema.Entry("related-property", allowed_types=[str]),
        Schema.Entry("separator", allowed_types=[str]),
        Schema.Entry("setter", allowed_types=[str]),
        Schema.Entry("settings-flag", allowed_types=[str]),
        Schema.Entry("sink-priority", allowed_types=[bool], default_value=False),
        Schema.Entry("skip-builder", allowed_types=[bool], default_value=False),
        Schema.Entry("skip-codegen", allowed_types=[bool], default_value=False),
        Schema.Entry("status", allowed_types=[str]),
        Schema.Entry("svg", allowed_types=[bool], default_value=False),
        Schema.Entry("synonym", allowed_types=[str]),
        Schema.Entry("top-priority", allowed_types=[bool], default_value=False),
        Schema.Entry("url", allowed_types=[str]),
        Schema.Entry("visited-link-color-support", allowed_types=[bool], default_value=False),
    )

    def __init__(self, **dictionary):
        CodeGenProperties.schema.set_attributes_from_dictionary(dictionary, instance=self)

    def __str__(self):
        return f"CodeGenProperties {vars(self)}"

    def __repr__(self):
        return self.__str__()

    @staticmethod
    def from_json(parsing_context, key_path, name, json_value):
        if type(json_value) is list:
            json_value = parsing_context.select_enabled_variant(json_value, label=f"{key_path}.codegen-properties")

        assert(type(json_value) is dict)
        CodeGenProperties.schema.validate_dictionary(json_value, label=f"CodeGenProperties ({key_path}.codegen-properties)")

        if json_value.get("top-priority", False):
            if json_value.get("comment") is None:
                raise Exception(f"{key_path} has top priority, but no comment to justify.")
            if json_value.get("longhands") is not None:
                raise Exception(f"{key_path} is a shorthand, but has top priority.")
            if json_value.get("high-priority", False):
                raise Exception(f"{key_path} can't have conflicting top/high priority.")

        if json_value.get("high-priority", False):
            if json_value.get("longhands") is not None:
                raise Exception(f"{key_path} is a shorthand, but has high priority.")

        if json_value.get("sink-priority", False):
            if json_value.get("longhands") is not None:
                raise Exception(f"{key_path} is a shorthand, but has sink priority.")

        if json_value.get("logical-property-group") is not None:
            if json_value.get("longhands") is not None:
                raise Exception(f"{key_path} is a shorthand, but belongs to a logical property group.")

        if "logical-property-group" in json_value:
            if json_value.get("longhands") is not None:
                raise Exception(f"{key_path} is a shorthand, but belongs to a logical property group.")
            json_value["logical-property-group"] = LogicalPropertyGroup.from_json(parsing_context, f"{key_path}.codegen-properties", json_value["logical-property-group"])

        if "computable" in json_value:
            if json_value["computable"]:
                if json_value.get("internal-only", False):
                    raise Exception(f"{key_path} can't be both internal-only and computable.")
        else:
            if json_value.get("internal-only", False):
                json_value["computable"] = False
            else:
                json_value["computable"] = True

        if json_value.get("related-property") is not None:
            if json_value.get("related-property") == name:
                raise Exception(f"{key_path} can't have itself as a related property.")
            if json_value.get("longhands") is not None:
                raise Exception(f"{key_path} can't have both a related property and be a shorthand.")
            if json_value.get("high-priority", False):
                raise Exception(f"{key_path} can't have both a related property and be high priority.")

        return CodeGenProperties(**json_value)

    @property
    def is_logical(self):
        if not self.logical_property_group:
            return False

        resolver = self.logical_property_group.resolver
        for logical_resolvers in LogicalPropertyGroup.logical_property_group_resolvers["logical"].values():
            for logical_resolver in logical_resolvers:
                if resolver == logical_resolver:
                    return True
        return False

    @property
    def is_deferred(self):
        return self.related_property or self.logical_property_group


class Property:
    schema = Schema(
        Schema.Entry("animatable", allowed_types=[bool], default_value=False),
        Schema.Entry("codegen-properties", allowed_types=[dict, list]),
        Schema.Entry("inherited", allowed_types=[bool], default_value=False),
        Schema.Entry("specification", allowed_types=[dict]),
        Schema.Entry("status", allowed_types=[dict, str]),
        Schema.Entry("values", allowed_types=[list]),
    )

    def __init__(self, name, **dictionary):
        self.name = name
        Property.schema.set_attributes_from_dictionary(dictionary, instance=self)

    def __str__(self):
        return self.name

    def __repr__(self):
        return self.__str__()

    @staticmethod
    def from_json(parsing_context, key_path, name, json_value):
        assert(type(json_value) is dict)
        Property.schema.validate_dictionary(json_value, label=f"Property ({key_path}.{name})")

        json_value["codegen-properties"] = CodeGenProperties.from_json(parsing_context, f"{key_path}.{name}", name, json_value.get("codegen-properties", {}))

        if "values" in json_value:
            json_value["values"] = list(filter(lambda value: value is not None, map(lambda value: Value.from_json(parsing_context, f"{key_path}.{name}", value), json_value["values"])))
        if "status" in json_value:
            json_value["status"] = Status.from_json(parsing_context, f"{key_path}.{name}", json_value["status"])
        if "specification" in json_value:
            json_value["specification"] = Specification.from_json(parsing_context, f"{key_path}.{name}", json_value["specification"])

        if parsing_context.parsing_for_codegen:
            if "codegen-properties" in json_value:
                if json_value["codegen-properties"].enable_if is not None and not parsing_context.is_enabled(conditional=json_value["codegen-properties"].enable_if):
                    if parsing_context.verbose:
                        print(f"SKIPPED {name} due to failing to satisfy 'enable-if' condition, '{json_value['codegen-properties'].enable_if}', with active macro set")
                    return None
                if json_value["codegen-properties"].skip_codegen is not None and json_value["codegen-properties"].skip_codegen:
                    if parsing_context.verbose:
                        print(f"SKIPPED {name} due to 'skip-codegen'")
                    return None

        return Property(name, **json_value)

    def perform_fixups_for_longhands(self, all_properties):
        # If 'longhands' was specified, replace the names with references to the Property objects.
        if self.codegen_properties.longhands:
            self.codegen_properties.longhands = [all_properties.properties_by_name[longhand_name] for longhand_name in self.codegen_properties.longhands]

    def perform_fixups_for_related_property(self, all_properties):
        # If 'related-property' was specified, validate the relationship and replace the name with a reference to the Property object.
        if self.codegen_properties.related_property:
            if self.codegen_properties.related_property not in all_properties.properties_by_name:
                raise Exception(f"Property {self.name} has an unknown related property: {self.codegen_properties.related_property}.")

            related_property = all_properties.properties_by_name[self.codegen_properties.related_property]
            if type(related_property.codegen_properties.related_property) is str:
                if related_property.codegen_properties.related_property != self.name:
                    raise Exception(f"Property {self.name} has {related_property.name} as a related property, but it's not reciprocal 1.")
            else:
                if related_property.codegen_properties.related_property.name != self.name:
                    raise Exception(f"Property {self.name} has {related_property.name} as a related property, but it's not reciprocal 2.")
            self.codegen_properties.related_property = related_property

    def perform_fixups_for_logical_property_group(self, all_properties):
        if self.codegen_properties.logical_property_group:
            group_name = self.codegen_properties.logical_property_group.name
            resolver = self.codegen_properties.logical_property_group.resolver
            kind = self.codegen_properties.logical_property_group.kind
            logic = self.codegen_properties.logical_property_group.logic

            all_properties.logical_property_groups.setdefault(group_name, {})

            existing_kind = all_properties.logical_property_groups[group_name].get("kind")
            if existing_kind and existing_kind != kind:
                raise Exception(f"Logical property group \"{group_name}\" has resolvers of different kinds: {kind} and {existing_kind}.")

            all_properties.logical_property_groups[group_name]["kind"] = kind

            existing_logic = all_properties.logical_property_groups[group_name].get(logic)
            if existing_logic:
                existing_property = existing_logic.get(resolver)
                if existing_property:
                    raise Exception(f"Logical property group \"{group_name}\" has multiple \"{resolver}\" properties: {self.name} and {existing_property.name}.")
            all_properties.logical_property_groups[group_name].setdefault(logic, {})
            all_properties.logical_property_groups[group_name][logic][resolver] = self

    def perform_fixups(self, all_properties):
        self.perform_fixups_for_longhands(all_properties)
        self.perform_fixups_for_related_property(all_properties)
        self.perform_fixups_for_logical_property_group(all_properties)

    def convert_name_to_id(name):
        return re.sub(r'(^[^-])|-(.)', lambda m: (m[1] or m[2]).upper(), name)

    @property
    @functools.cache
    def id(self):
        return f"CSSProperty{Property.convert_name_to_id(self.name)}"

    @property
    def aliases(self):
        return self.codegen_properties.aliases

    @property
    def is_skipped_from_computed_style(self):
        if self.codegen_properties.internal_only:
            return True

        if not self.codegen_properties.computable:
            return True

        if self.codegen_properties.skip_builder and not self.codegen_properties.is_logical:
            return True

        if self.codegen_properties.longhands is not None:
            for longhand in self.codegen_properties.longhands:
                if not longhand.is_skipped_from_computed_style:
                    return True

        return False


class Properties:
    schema = Schema(
        Schema.Entry("categories", allowed_types=[dict]),
        Schema.Entry("instructions", allowed_types=[list]),
        Schema.Entry("properties", allowed_types=[dict]),
    )

    def __init__(self, *properties):
        self.properties = properties
        self.properties_by_name = {property.name: property for property in properties}
        self.logical_property_groups = {}

    def __str__(self):
        return "Properties"

    def __repr__(self):
        return self.__str__()

    @staticmethod
    def from_json(parsing_context, top_level_object):
        Properties.schema.validate_dictionary(top_level_object, label="top level object")

        properties = Properties(
            *list(
                filter(
                    lambda value: value is not None,
                    map(
                        lambda item: Property.from_json(parsing_context, "$properties", item[0], item[1]),
                        top_level_object["properties"].items()
                    )
                )
            )
        )

        for property in properties.properties:
            property.perform_fixups(properties)

        return properties

    @property
    @functools.cache
    def settings_flags(self):
        return sorted(list(set([property.codegen_properties.settings_flag for property in self.properties if property.codegen_properties.settings_flag])))

    @property
    @functools.cache
    def all(self):
        return sorted(self.properties, key=functools.cmp_to_key(Properties._sort_by_descending_priority_and_name))

    @property
    @functools.cache
    def all_with_prefixed_properties_last(self):
        return sorted(self.properties, key=functools.cmp_to_key(Properties._sort_with_prefixed_properties_last))

    @property
    @functools.cache
    def all_with_settings_flag(self):
        return sorted([property for property in self.all if property.codegen_properties.settings_flag], key=lambda property: property.name)

    @property
    @functools.cache
    def all_direction_aware_properties(self):
        for group_name, property_group in sorted(self.logical_property_groups.items(), key=lambda x: x[0]):
            for resolver, property in sorted(property_group["logical"].items(), key=lambda x: x[1].name):
                yield property

    @property
    @functools.cache
    def all_in_logical_property_group(self):
        for group_name, property_group in sorted(self.logical_property_groups.items(), key=lambda x: x[0]):
            for kind in ["logical", "physical"]:
                for resolver, property in sorted(property_group[kind].items(), key=lambda x: x[1].name):
                    yield property

    @property
    @functools.cache
    def internal_only(self):
        return sorted([property for property in self.all if property.codegen_properties.internal_only], key=lambda property: property.name)

    def _sort_by_descending_priority_and_name(a, b):
        # Sort shorthands to the back
        a_is_shorthand = a.codegen_properties.longhands is not None
        b_is_shorthand = b.codegen_properties.longhands is not None
        if a_is_shorthand and not b_is_shorthand:
            return 1
        if not a_is_shorthand and b_is_shorthand:
            return -1

        # Sort longhands with top priority to the front
        a_is_top_priority = a.codegen_properties.top_priority
        b_is_top_priority = b.codegen_properties.top_priority
        if a_is_top_priority and not b_is_top_priority:
            return -1
        if not a_is_top_priority and b_is_top_priority:
            return 1

        # Sort longhands with high priority to the front
        a_is_high_priority = a.codegen_properties.high_priority
        b_is_high_priority = b.codegen_properties.high_priority
        if a_is_high_priority and not b_is_high_priority:
            return -1
        if not a_is_high_priority and b_is_high_priority:
            return 1

        # Sort deferred longhands to the back, before shorthands
        a_is_deferred = a.codegen_properties.is_deferred
        b_is_deferred = b.codegen_properties.is_deferred
        if a_is_deferred and not b_is_deferred:
            return 1
        if not a_is_deferred and b_is_deferred:
            return -1

        # Sort sunken names at the end of their priority bucket
        a_is_sink_priority = a.codegen_properties.sink_priority
        b_is_sink_priority = b.codegen_properties.sink_priority
        if a_is_sink_priority and not b_is_sink_priority:
            return 1
        if not a_is_sink_priority and b_is_sink_priority:
            return -1

        # Sort names without leading '-' to the front
        a_starts_with_prefix = a.name[0] == "-"
        b_starts_with_prefix = b.name[0] == "-"
        if a_starts_with_prefix and not b_starts_with_prefix:
            return 1
        if not a_starts_with_prefix and b_starts_with_prefix:
            return -1

        if a.name == b.name:
            return 0
        return a.name < b.name

    def _sort_with_prefixed_properties_last(a, b):
        a_starts_with_prefix = a.name[0] == "-"
        b_starts_with_prefix = b.name[0] == "-"
        if a_starts_with_prefix and not b_starts_with_prefix:
            return 1
        if not a_starts_with_prefix and b_starts_with_prefix:
            return -1
        if a.name == b.name:
            return 0
        return a.name < b.name


# GENERATION

def quoted(iterable, suffix=""):
    return [f'"{x}"{suffix}' for x in iterable]


class GenerationContext:
    def __init__(self, properties, *, verbose, gperf_executable):
        self.properties = properties
        self.verbose = verbose
        self.gperf_executable = gperf_executable

    # Shared generation constants.
    number_of_predefined_properties = 2

    def run_gperf(self):
        gperf_command = self.gperf_executable or os.environ['GPERF']

        gperf_result_code = subprocess.call([gperf_command, '--key-positions=*', '-D', '-n', '-s', '2', 'CSSPropertyNames.gperf', '--output-file=CSSPropertyNames.cpp'])
        if gperf_result_code != 0:
            raise Exception(f"Error when generating CSSPropertyNames.cpp from CSSPropertyNames.gperf: {gperf_result_code}")

    def _generate_gperf_prefix(self, *, to):
        to.write(textwrap.dedent("""\
            %{
            // This file is automatically generated from CSSProperties.json by the makeprop.pl script. Do not edit it.

            #include "config.h"
            #include "CSSPropertyNames.h"

            #include "CSSProperty.h"
            #include "Settings.h"
            #include <wtf/ASCIICType.h>
            #include <wtf/Hasher.h>
            #include <wtf/text/AtomString.h>
            #include <string.h>

            IGNORE_WARNINGS_BEGIN(\"implicit-fallthrough\")

            // Older versions of gperf like to use the `register` keyword.
            #define register

            namespace WebCore {

            // Using std::numeric_limits<uint16_t>::max() here would be cleaner,
            // but is not possible due to missing constexpr support in MSVC 2013.
            static_assert(numCSSProperties + 1 <= 65535, "CSSPropertyID should fit into uint16_t.");

            """))

        all_computed_property_ids = [f"{property.id}," for property in self.properties.all_with_prefixed_properties_last if not property.is_skipped_from_computed_style]
        to.write(f"const std::array<CSSPropertyID, {len(all_computed_property_ids)}> computedPropertyIDs {{")
        to.write("\n    ")
        to.write("\n    ".join(all_computed_property_ids))
        to.write("\n};\n\n")

        all_property_name_strings = quoted(self.properties.all, "_s,")
        to.write(f"constexpr ASCIILiteral propertyNameStrings[numCSSProperties] = {{")
        to.write("\n    ")
        to.write("\n    ".join(all_property_name_strings))
        to.write("\n};\n\n")

        to.write("%}\n")

    def _generate_gperf_definition(self, *, to):
        to.write(textwrap.dedent("""\
            %struct-type
            struct CSSPropertyHashTableEntry {
                const char* name;
                uint16_t id;
            };
            %language=C++
            %readonly-tables
            %global-table
            %7bit
            %compare-strncmp
            %define class-name CSSPropertyNamesHash
            %enum
            %%
            """))

        all_property_names_and_ids = itertools.chain(
            [f'{property.name}, {property.id}' for property in self.properties.all],
            itertools.chain.from_iterable([[f'{alias}, {property.id}' for alias in property.aliases] for property in self.properties.all])
        )
        to.write("\n".join(all_property_names_and_ids))

        to.write("\n%%\n")

    def _generate_lookup_functions(self, *, to):
        to.write(textwrap.dedent("""
            CSSPropertyID findCSSProperty(const char* characters, unsigned length)
            {
                auto* value = CSSPropertyNamesHash::in_word_set(characters, length);
                return value ? static_cast<CSSPropertyID>(value->id) : CSSPropertyInvalid;
            }

            ASCIILiteral nameLiteral(CSSPropertyID id)
            {
                if (id < firstCSSProperty)
                    return { };
                unsigned index = id - firstCSSProperty;
                if (index >= numCSSProperties)
                    return { };
                return propertyNameStrings[index];
            }

            const AtomString& nameString(CSSPropertyID id)
            {
                if (id < firstCSSProperty)
                    return nullAtom();
                unsigned index = id - firstCSSProperty;
                if (index >= numCSSProperties)
                    return nullAtom();

                static NeverDestroyed<std::array<AtomString, numCSSProperties>> atomStrings;
                auto& string = atomStrings.get()[index];
                if (string.isNull())
                    string = propertyNameStrings[index];
                return string;
            }

            String nameForIDL(CSSPropertyID id)
            {
                LChar characters[maxCSSPropertyNameLength];
                const char* nameForCSS = nameLiteral(id);
                if (!nameForCSS)
                    return emptyString();

                auto* propertyNamePointer = nameForCSS;
                auto* nextCharacter = characters;
                while (char character = *propertyNamePointer++) {
                    if (character == '-') {
                        char nextCharacter = *propertyNamePointer++;
                        if (!nextCharacter)
                            break;
                        character = (propertyNamePointer - 2 != nameForCSS) ? toASCIIUpper(nextCharacter) : nextCharacter;
                    }
                    *nextCharacter++ = character;
                }
                unsigned length = nextCharacter - characters;
                return { characters, length };
            }

            """))

    def _generate_property_id_switch_function(self, *, to, signature, properties, mapping, default, prologue=None, epilogue=None):
        to.write(f"{signature}\n")
        to.write(f"{{\n")

        if prologue:
            to.write(textwrap.indent(prologue, '    ') + "\n")

        to.write(f"    switch (id) {{\n")

        for property in properties:
            to.write(f"    case CSSPropertyID::{property.id}:\n")
            to.write(f"        {mapping(property)}\n")

        to.write(f"    default:\n")
        to.write(f"        {default}\n")
        to.write(f"    }}\n")

        if epilogue:
            to.write(textwrap.indent(epilogue, '    ') + "\n")

        to.write(f"}}\n\n")

    def _generate_property_id_switch_function_bool(self, *, to, signature, properties):
        to.write(f"{signature}\n")
        to.write(f"{{\n")
        to.write(f"    switch (id) {{\n")

        for property in properties:
            to.write(f"    case CSSPropertyID::{property.id}:\n")

        to.write(f"        return true;\n")
        to.write(f"    default:\n")
        to.write(f"        return false;\n")
        to.write(f"    }}\n")
        to.write(f"}}\n\n")

    def _generate_physical_logical_conversion_function(self, *, to, signature, source, destination, resolver_enum_prefix):
        source_as_id = Property.convert_name_to_id(source)
        destination_as_id = Property.convert_name_to_id(destination)

        to.write(f"{signature}\n")
        to.write(f"{{\n")
        to.write(f"    auto textflow = makeTextFlow(writingMode, direction);\n")
        to.write(f"    switch (id) {{\n")

        for group_name, property_group in sorted(self.properties.logical_property_groups.items(), key=lambda x: x[0]):
            kind = property_group["kind"]
            kind_as_id = Property.convert_name_to_id(kind)

            destinations = LogicalPropertyGroup.logical_property_group_resolvers[destination][kind]
            properties = [property_group[destination][a_destination].id for a_destination in destinations]

            for resolver, property in sorted(property_group[source].items(), key=lambda x: x[0]):
                resolver_as_id = Property.convert_name_to_id(resolver)
                resolver_enum = f"{resolver_enum_prefix}{kind_as_id}::{resolver_as_id}"

                to.write(f"    case CSSPropertyID::{property.id}: {{\n")
                to.write(f"        static constexpr CSSPropertyID properties[{len(properties)}] = {{ {', '.join(properties)} }};\n")
                to.write(f"        return properties[static_cast<size_t>(map{source_as_id}{kind_as_id}To{destination_as_id}{kind_as_id}(textflow, {resolver_enum}))];\n")
                to.write(f"    }}\n")
        to.write(f"    default:\n")
        to.write(f"        return false;\n")
        to.write(f"    }}\n")
        to.write(f"}}\n\n")

    def _generate_is_inherited_property(self, *, to):
        to.write(f'constexpr bool isInheritedPropertyTable[numCSSProperties + {GenerationContext.number_of_predefined_properties}] = {{\n')
        to.write(f'    false, // CSSPropertyInvalid\n')
        to.write(f'    true , // CSSPropertyCustom\n')

        all_inherited_and_ids = [f'    {"true " if property.inherited else "false"}, // {property.id}' for property in self.properties.all]

        to.write(f'\n'.join(all_inherited_and_ids))
        to.write(f'}};\n\n')

        to.write(f"bool CSSProperty::isInheritedProperty(CSSPropertyID id)\n")
        to.write(f"{{\n")
        to.write(f"    ASSERT(id < numCSSProperties);\n")
        to.write(f"    ASSERT(id != CSSPropertyInvalid);\n")
        to.write(f"    return isInheritedPropertyTable[id];\n")
        to.write(f"}}\n\n")

    def _generate_are_in_same_logical_property_group_with_different_mappings_logic(self, *, to):
        to.write(f"bool CSSProperty::areInSameLogicalPropertyGroupWithDifferentMappingLogic(CSSPropertyID id1, CSSPropertyID id2)\n")
        to.write(f"{{\n")
        to.write(f"    switch (id1) {{\n")

        for group_name, property_group in sorted(self.properties.logical_property_groups.items(), key=lambda x: x[0]):
            logical = property_group["logical"]
            physical = property_group["physical"]
            for first in [logical, physical]:
                second = physical if first is logical else logical
                for resolver, property in sorted(first.items(), key=lambda x: x[1].name):
                    to.write(f"    case CSSPropertyID::{property.id}:\n")

                to.write(f"        switch (id2) {{\n")
                for resolver, property in sorted(second.items(), key=lambda x: x[1].name):
                    to.write(f"        case CSSPropertyID::{property.id}:\n")

                to.write(f"            return true;\n")
                to.write(f"        default:\n")
                to.write(f"            return false;\n")
                to.write(f"        }}\n")

        to.write(f"    default:\n")
        to.write(f"        return false;\n")
        to.write(f"    }}\n")
        to.write(f"}}\n\n")

    def _generate_css_property_settings_constructor(self, *, to):
        to.write(f"CSSPropertySettings::CSSPropertySettings(const Settings& settings)\n")
        to.write(f"    : ")

        settings_initializer_list = [f"{flag} {{ settings.{flag}() }}" for flag in self.properties.settings_flags]

        to.write(f"\n    , ".join(settings_initializer_list))
        to.write(f"{{\n")
        to.write(f"}}\n\n")

    def _generate_css_property_settings_operator_equal(self, *, to):
        to.write(f"bool operator==(const CSSPropertySettings& a, const CSSPropertySettings& b)\n")
        to.write(f"{{\n")

        settings_operator_equal_list = [f"a.{flag} == b.{flag}" for flag in self.properties.settings_flags]
        to.write(f"\n        && ".join(settings_operator_equal_list))
        to.write(f";\n")
        to.write(f"}}\n\n")

    def _generate_css_property_settings_hasher(self, *, to):
        to.write(f"void add(Hasher& hasher, const CSSPropertySettings& settings)\n")
        to.write(f"{{\n")
        to.write(f"    unsigned bits = ")

        settings_hasher_list = [f"settings.{flag} << {i}" for (i, flag) in enumerate(self.properties.settings_flags)]
        to.write(f"\n        | ".join(settings_hasher_list))
        to.write(f";\n")
        to.write(f"    add(hasher, bits);\n")
        to.write(f"}}\n\n")

    def generate_css_property_names_gperf(self):
        with open('CSSPropertyNames.gperf', 'w') as output_file:
            self._generate_gperf_prefix(
                to=output_file
            )

            self._generate_gperf_definition(
                to=output_file
            )

            self._generate_lookup_functions(
                to=output_file
            )

            self._generate_property_id_switch_function_bool(
                to=output_file,
                signature="bool isInternal(CSSPropertyID id)",
                properties=self.properties.internal_only
            )

            self._generate_property_id_switch_function(
                to=output_file,
                signature="bool isExposed(CSSPropertyID id, const Settings* settings)",
                properties=self.properties.all_with_settings_flag,
                mapping=lambda p: f"return settings->{p.codegen_properties.settings_flag}();",
                default="return true;",
                prologue=textwrap.dedent("""\
                    if (id == CSSPropertyInvalid || isInternal(id))
                        return false;

                    if (!settings)
                        return true;
                """)
            )

            self._generate_property_id_switch_function(
                to=output_file,
                signature="bool isExposed(CSSPropertyID id, const CSSPropertySettings* settings)",
                properties=self.properties.all_with_settings_flag,
                mapping=lambda p: f"return settings->{p.codegen_properties.settings_flag};",
                default="return true;",
                prologue=textwrap.dedent("""\
                    if (id == CSSPropertyInvalid || isInternal(id))
                        return false;

                    if (!settings)
                        return true;
                """)
            )

            self._generate_is_inherited_property(
                to=output_file
            )

            self._generate_property_id_switch_function(
                to=output_file,
                signature="CSSPropertyID relatedProperty(CSSPropertyID id)",
                properties=[p for p in self.properties.all if p.codegen_properties.related_property],
                mapping=lambda p: f"return CSSPropertyID::{p.codegen_properties.related_property.id};",
                default="return CSSPropertyID::CSSPropertyInvalid;"
            )

            self._generate_property_id_switch_function(
                to=output_file,
                signature="Vector<String> CSSProperty::aliasesForProperty(CSSPropertyID id)",
                properties=[p for p in self.properties.all if p.codegen_properties.aliases],
                mapping=lambda p: f"return {{ {', '.join(quoted(p.codegen_properties.aliases, '_s'))} }};",
                default="return { };"
            )

            self._generate_property_id_switch_function_bool(
                to=output_file,
                signature="bool CSSProperty::isColorProperty(CSSPropertyID id)",
                properties=[p for p in self.properties.all if p.codegen_properties.color_property]
            )

            self._generate_property_id_switch_function(
                to=output_file,
                signature="UChar CSSProperty::listValuedPropertySeparator(CSSPropertyID id)",
                properties=[p for p in self.properties.all if p.codegen_properties.separator],
                mapping=lambda p: f"return '{ p.codegen_properties.separator[0] }';",
                default="break;",
                epilogue="return '\\0';"
            )

            self._generate_property_id_switch_function_bool(
                to=output_file,
                signature="bool CSSProperty::isDirectionAwareProperty(CSSPropertyID id)",
                properties=self.properties.all_direction_aware_properties
            )

            self._generate_property_id_switch_function_bool(
                to=output_file,
                signature="bool CSSProperty::isInLogicalPropertyGroup(CSSPropertyID id)",
                properties=self.properties.all_in_logical_property_group
            )

            self._generate_are_in_same_logical_property_group_with_different_mappings_logic(
                to=output_file
            )

            self._generate_physical_logical_conversion_function(
                to=output_file,
                signature="CSSPropertyID CSSProperty::resolveDirectionAwareProperty(CSSPropertyID id, TextDirection direction, WritingMode writingMode)",
                source="logical",
                destination="physical",
                resolver_enum_prefix="LogicalBox"
            )

            self._generate_physical_logical_conversion_function(
                to=output_file,
                signature="CSSPropertyID CSSProperty::unresolvePhysicalProperty(CSSPropertyID id, TextDirection direction, WritingMode writingMode)",
                source="physical",
                destination="logical",
                resolver_enum_prefix="Box"
            )

            self._generate_property_id_switch_function_bool(
                to=output_file,
                signature="bool CSSProperty::isDescriptorOnly(CSSPropertyID id)",
                properties=[p for p in self.properties.all if p.codegen_properties.descriptor_only]
            )

            self._generate_css_property_settings_constructor(
                to=output_file
            )

            self._generate_css_property_settings_operator_equal(
                to=output_file
            )

            self._generate_css_property_settings_hasher(
                to=output_file
            )

            output_file.write(textwrap.dedent("""\
                } // namespace WebCore

                IGNORE_WARNINGS_END
                """))

    def generate_css_property_names_h(self):
        pass

    def generate_css_style_declaration_property_names_idl(self):
        pass

    def generate_style_builder_generated_cpp(self):
        pass

    def generate_style_property_shorthand_functions_h(self):
        pass

    def generate_style_property_shorthand_functions_cpp(self):
        pass


def main():
    parser = argparse.ArgumentParser(description='Process CSS property definitions.')
    parser.add_argument('--properties', default="CSSProperties.json")
    parser.add_argument('--defines')
    parser.add_argument('--gperf-executable')
    parser.add_argument('-v', '--verbose', action='store_true')
    args = parser.parse_args()

    with open(args.properties, "r") as properties_file:
        properties_json = json.load(properties_file)

    parsing_context = ParsingContext(defines_string=args.defines, parsing_for_codegen=True, verbose=args.verbose)
    properties = Properties.from_json(parsing_context, properties_json)

    if args.verbose:
        print(f"{len(properties.properties)} properties active for code generation")

    generation_context = GenerationContext(properties, verbose=args.verbose, gperf_executable=args.gperf_executable)

    generation_context.generate_css_property_names_gperf()
    generation_context.run_gperf()

    generation_context.generate_css_property_names_h()
    generation_context.generate_css_style_declaration_property_names_idl()

    generation_context.generate_style_builder_generated_cpp()
    generation_context.generate_style_property_shorthand_functions_h()
    generation_context.generate_style_property_shorthand_functions_cpp()


if __name__ == "__main__":
    main()
