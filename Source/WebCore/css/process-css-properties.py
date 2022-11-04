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
import json
import os
import subprocess
import sys


def is_conditional_enabled(parsing_context, conditional):
    if conditional[0] == '!':
        if conditional[1:] not in parsing_context.conditionals:
            return True
    else:
        if conditional in parsing_context.conditionals:
            return True
    return False


def select_enabled_variant(parsing_context, variants, label):
    for variant in variants:
        if "enable-if" not in variant:
            raise Exception(f"Invalid conditional definition for '{label}'. No 'enable-if' property found.")

        if is_conditional_enabled(parsing_context, variant["enable-if"]):
            return variant

    raise Exception(f"Invalid conditional definition for '{label}'. No 'enable-if' property matched the active set.")


class Schema:
    class Entry:
        def __init__(self, key, *, allowed_types):
            self.key = key
            self.allowed_types = allowed_types
            self.default_value = None

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
        Schema.Entry("keywords", allowed_types=[list]),
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

        if "enable-if" in json_value and not is_conditional_enabled(parsing_context, json_value["enable-if"]):
            return None

        return Value(**json_value)


class LogicalPropertyGroup:
    schema = Schema(
        Schema.Entry("name", allowed_types=[str]),
        Schema.Entry("resolver", allowed_types=[str]),
    )

    def __init__(self, **dictionary):
        LogicalPropertyGroup.schema.set_attributes_from_dictionary(dictionary, instance=self)

    def __str__(self):
        return f"LogicalPropertyGroup {vars(self)}"

    def __repr__(self):
        return self.__str__()

    @staticmethod
    def from_json(parsing_context, key_path, json_value):
        assert(type(json_value) is dict)
        LogicalPropertyGroup.schema.validate_dictionary(json_value, label=f"LogicalPropertyGroup ({key_path}.logical-property-group)")
        return LogicalPropertyGroup(**json_value)


class CodeGenProperties:
    schema = Schema(
        Schema.Entry("aliases", allowed_types=[list]),
        Schema.Entry("auto-functions", allowed_types=[bool]),
        Schema.Entry("color-property", allowed_types=[bool]),
        Schema.Entry("comment", allowed_types=[str]),
        Schema.Entry("computable", allowed_types=[bool]),
        Schema.Entry("conditional-converter", allowed_types=[str]),
        Schema.Entry("converter", allowed_types=[str]),
        Schema.Entry("custom", allowed_types=[str]),
        Schema.Entry("descriptor-only", allowed_types=[bool]),
        Schema.Entry("enable-if", allowed_types=[str]),
        Schema.Entry("fast-path-inherited", allowed_types=[bool]),
        Schema.Entry("fill-layer-property", allowed_types=[bool]),
        Schema.Entry("font-property", allowed_types=[bool]),
        Schema.Entry("getter", allowed_types=[str]),
        Schema.Entry("high-priority", allowed_types=[bool]),
        Schema.Entry("initial", allowed_types=[str]),
        Schema.Entry("internal-only", allowed_types=[bool]),
        Schema.Entry("logical-property-group", allowed_types=[dict]),
        Schema.Entry("longhands", allowed_types=[list]),
        Schema.Entry("name-for-methods", allowed_types=[str]),
        Schema.Entry("related-property", allowed_types=[str]),
        Schema.Entry("separator", allowed_types=[str]),
        Schema.Entry("setter", allowed_types=[str]),
        Schema.Entry("settings-flag", allowed_types=[str]),
        Schema.Entry("sink-priority", allowed_types=[bool]),
        Schema.Entry("skip-builder", allowed_types=[bool]),
        Schema.Entry("skip-codegen", allowed_types=[bool]),
        Schema.Entry("status", allowed_types=[str]),
        Schema.Entry("svg", allowed_types=[bool]),
        Schema.Entry("synonym", allowed_types=[str]),
        Schema.Entry("top-priority", allowed_types=[bool]),
        Schema.Entry("url", allowed_types=[str]),
        Schema.Entry("visited-link-color-support", allowed_types=[bool]),
    )

    def __init__(self, **dictionary):
        CodeGenProperties.schema.set_attributes_from_dictionary(dictionary, instance=self)

    def __str__(self):
        return f"CodeGenProperties {vars(self)}"

    def __repr__(self):
        return self.__str__()

    @staticmethod
    def from_json(parsing_context, key_path, json_value):
        if type(json_value) is list:
            json_value = select_enabled_variant(parsing_context, json_value, f"{key_path}.codegen-properties")

        assert(type(json_value) is dict)
        CodeGenProperties.schema.validate_dictionary(json_value, label=f"CodeGenProperties ({key_path}.codegen-properties)")

        if "logical-property-group" in json_value:
            json_value["logical-property-group"] = LogicalPropertyGroup.from_json(parsing_context, f"{key_path}.codegen-properties", json_value["logical-property-group"])

        result = CodeGenProperties(**json_value)

        if result.top_priority:
            if result.comment is None:
                raise Exception(f"{key_path} has top priority, but no comment to justify.")
            if result.longhands is not None:
                raise Exception(f"{key_path} is a shorthand, but has top priority.")
            if result.high_priority:
                raise Exception(f"{key_path} can't have conflicting top/high priority.")

        if result.high_priority:
            if result.longhands is not None:
                raise Exception(f"{key_path} is a shorthand, but has high priority.")

        if result.sink_priority:
            if result.longhands is not None:
                raise Exception(f"{key_path} is a shorthand, but has sink priority.")

        if result.logical_property_group is not None:
            if result.longhands is not None:
                raise Exception(f"{key_path} is a shorthand, but belongs to a logical property group.")

        return result


class Property:
    schema = Schema(
        Schema.Entry("animatable", allowed_types=[bool]),
        Schema.Entry("codegen-properties", allowed_types=[dict, list]),
        Schema.Entry("inherited", allowed_types=[bool]),
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

        if "values" in json_value:
            json_value["values"] = list(filter(lambda value: value is not None, map(lambda value: Value.from_json(parsing_context, f"{key_path}.{name}", value), json_value["values"])))
        if "codegen-properties" in json_value:
            json_value["codegen-properties"] = CodeGenProperties.from_json(parsing_context, f"{key_path}.{name}", json_value["codegen-properties"])
        if "status" in json_value:
            json_value["status"] = Status.from_json(parsing_context, f"{key_path}.{name}", json_value["status"])
        if "specification" in json_value:
            json_value["specification"] = Specification.from_json(parsing_context, f"{key_path}.{name}", json_value["specification"])

        if parsing_context.parsing_for_codegen:
            if "codegen-properties" in json_value:
                if json_value["codegen-properties"].enable_if is not None and not is_conditional_enabled(parsing_context, json_value["codegen-properties"].enable_if):
                    if parsing_context.verbose:
                        print(f"SKIPPED {name} due to failing to satisfy 'enable-if' condition, '{json_value['codegen-properties'].enable_if}', with active macro set")
                    return None
                if json_value["codegen-properties"].skip_codegen is not None and json_value["codegen-properties"].skip_codegen:
                    if parsing_context.verbose:
                        print(f"SKIPPED {name} due to 'skip-codegen'")
                    return None

        return Property(name, **json_value)


class ParsingContext:
    def __init__(self, *, defines_string, parsing_for_codegen, verbose):
        if defines_string:
            self.conditionals = set(defines_string.split(' '))
        else:
            self.conditionals = set()
        self.parsing_for_codegen = parsing_for_codegen
        self.verbose = verbose


def top_level_from_json(parsing_context, top_level_object):
    top_level_object_schema = Schema(
        Schema.Entry("categories", allowed_types=[dict]),
        Schema.Entry("instructions", allowed_types=[list]),
        Schema.Entry("properties", allowed_types=[dict]),
    )

    top_level_object_schema.validate_dictionary(top_level_object, label="top level object")

    return list(filter(lambda value: value is not None, map(lambda item: Property.from_json(parsing_context, "$properties", item[0], item[1]), top_level_object["properties"].items())))


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

    properties = top_level_from_json(parsing_context, properties_json)

    if parsing_context.verbose:
        print(f"{len(properties)} properties active for code generation")


if __name__ == "__main__":
    main()
