#!/usr/bin/env python3
"""
Generate deterministic C schema tables for consensus-spec ssz_static fixtures.

The design is a compact generic schema registry: the generator discovers
(preset, fork, handler) tuples from the fixture tree, resolves each handler to a
pyspec type, structurally interns the reachable schemas, and emits one
deduplicated type table plus a registry mapping each fixture tuple to a root
type index. This keeps the runtime schema-driven and removes any need for
hand-authored beacon descriptors. Unions are intentionally unsupported for now:
the current fixture-discovered handler set does not include them, and the
generator will fail loudly if that changes.
"""

from __future__ import annotations

import argparse
import importlib
import sys
from dataclasses import dataclass
from pathlib import Path
from typing import Dict
from typing import List
from typing import Sequence
from typing import Tuple


SCHEMA_HEADER_GUARD = "TESTS_SPEC_GENERATED_SSZ_STATIC_SCHEMA_GENERATED_H"

NO_CHILD_INDEX = "UINT32_MAX"
DYNAMIC_SIZE = "SSZ_STATIC_SCHEMA_DYNAMIC_SIZE"


@dataclass(frozen=True)
class RegistryEntry:
    preset: str
    fork: str
    handler: str


@dataclass
class FieldEntry:
    child_type_index: int
    fixed_size: int | None


@dataclass
class TypeEntry:
    kind: str
    param: int
    child_type_index: int | None
    field_index: int
    field_count: int
    fixed_size: int | None
    min_size: int


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser()
    parser.add_argument("--fixtures-root", required=True, type=Path)
    parser.add_argument("--pyspec-root", required=True, type=Path)
    parser.add_argument("--output-header", required=True, type=Path)
    parser.add_argument("--output-source", required=True, type=Path)
    return parser.parse_args()


def ensure_pyspec_root(pyspec_root: Path) -> None:
    pyspec_root = pyspec_root.resolve()
    if str(pyspec_root) not in sys.path:
        sys.path.insert(0, str(pyspec_root))


def discover_registry_entries(fixtures_root: Path) -> List[RegistryEntry]:
    entries: List[RegistryEntry] = []

    for preset_dir in sorted(fixtures_root.iterdir()):
        if not preset_dir.is_dir():
            continue
        for fork_dir in sorted(preset_dir.iterdir()):
            static_dir = fork_dir / "ssz_static"
            if not static_dir.is_dir():
                continue
            for handler_dir in sorted(static_dir.iterdir()):
                if handler_dir.is_dir():
                    entries.append(
                        RegistryEntry(
                            preset=preset_dir.name,
                            fork=fork_dir.name,
                            handler=handler_dir.name,
                        )
                    )

    if not entries:
        raise RuntimeError(f"no ssz_static handlers found under {fixtures_root}")

    return entries


def import_schema_type(entry: RegistryEntry):
    module_name = f"eth_consensus_specs.{entry.fork}.{entry.preset}"
    module = importlib.import_module(module_name)

    try:
        schema_type = getattr(module, entry.handler)
    except AttributeError as exc:
        raise RuntimeError(
            f"handler {entry.handler!r} not found in pyspec module {module_name}"
        ) from exc

    if not isinstance(schema_type, type):
        raise RuntimeError(f"{module_name}.{entry.handler} is not a type")

    return schema_type


def format_u64(value: int) -> str:
    return f"UINT64_C({value})"


def format_size(value: int | None) -> str:
    if value is None:
        return DYNAMIC_SIZE
    return f"((size_t){format_u64(value)})"


class SchemaGenerator:
    def __init__(self) -> None:
        from remerkleable.basic import BasicView
        from remerkleable.basic import boolean
        from remerkleable.bitfields import Bitlist
        from remerkleable.bitfields import Bitvector
        from remerkleable.byte_arrays import ByteList
        from remerkleable.byte_arrays import ByteVector
        from remerkleable.complex import Container
        from remerkleable.complex import List as ComplexList
        from remerkleable.complex import Vector
        from remerkleable.union import Union

        self._basic_view = BasicView
        self._boolean = boolean
        self._bitlist = Bitlist
        self._bitvector = Bitvector
        self._byte_list = ByteList
        self._byte_vector = ByteVector
        self._container = Container
        self._list = ComplexList
        self._vector = Vector
        self._union = Union

        self._signature_to_index: Dict[Tuple[object, ...], int] = {}
        self.types: List[TypeEntry] = []
        self.fields: List[FieldEntry] = []

    def _classify_basic_kind(self, schema_type: type) -> str:
        if issubclass(schema_type, self._boolean):
            return "SSZ_STATIC_SCHEMA_KIND_BOOL"

        byte_length = int(schema_type.type_byte_length())
        kind_by_size = {
            1: "SSZ_STATIC_SCHEMA_KIND_UINT8",
            2: "SSZ_STATIC_SCHEMA_KIND_UINT16",
            4: "SSZ_STATIC_SCHEMA_KIND_UINT32",
            8: "SSZ_STATIC_SCHEMA_KIND_UINT64",
            16: "SSZ_STATIC_SCHEMA_KIND_UINT128",
            32: "SSZ_STATIC_SCHEMA_KIND_UINT256",
        }
        try:
            return kind_by_size[byte_length]
        except KeyError as exc:
            raise RuntimeError(
                f"unsupported basic type width {byte_length} for {schema_type}"
            ) from exc

    def _signature(self, schema_type: type) -> Tuple[object, ...]:
        if issubclass(schema_type, self._union):
            raise RuntimeError(f"unsupported union schema {schema_type}")

        if issubclass(schema_type, self._basic_view):
            return (self._classify_basic_kind(schema_type),)

        if issubclass(schema_type, self._byte_vector):
            return (
                "SSZ_STATIC_SCHEMA_KIND_BYTE_VECTOR",
                int(schema_type.vector_length()),
            )

        if issubclass(schema_type, self._byte_list):
            return (
                "SSZ_STATIC_SCHEMA_KIND_BYTE_LIST",
                int(schema_type.limit()),
            )

        if issubclass(schema_type, self._bitvector):
            return (
                "SSZ_STATIC_SCHEMA_KIND_BIT_VECTOR",
                int(schema_type.vector_length()),
            )

        if issubclass(schema_type, self._bitlist):
            return (
                "SSZ_STATIC_SCHEMA_KIND_BIT_LIST",
                int(schema_type.limit()),
            )

        if issubclass(schema_type, self._vector):
            return (
                "SSZ_STATIC_SCHEMA_KIND_VECTOR",
                int(schema_type.vector_length()),
                self._signature(schema_type.element_cls()),
            )

        if issubclass(schema_type, self._list):
            return (
                "SSZ_STATIC_SCHEMA_KIND_LIST",
                int(schema_type.limit()),
                self._signature(schema_type.element_cls()),
            )

        if issubclass(schema_type, self._container):
            return (
                "SSZ_STATIC_SCHEMA_KIND_CONTAINER",
                tuple(self._signature(field_type) for field_type in schema_type.fields().values()),
            )

        raise RuntimeError(f"unsupported schema type {schema_type}")

    @staticmethod
    def _bits_to_bytes(bit_count: int) -> int:
        return (bit_count + 7) // 8

    def _intern(self, schema_type: type) -> int:
        signature = self._signature(schema_type)
        cached = self._signature_to_index.get(signature)
        if cached is not None:
            return cached

        kind = str(signature[0])
        child_type_index: int | None = None
        field_index = 0
        field_count = 0
        fixed_size: int | None = None
        min_size = 0
        param = 0

        if kind in (
            "SSZ_STATIC_SCHEMA_KIND_BOOL",
            "SSZ_STATIC_SCHEMA_KIND_UINT8",
            "SSZ_STATIC_SCHEMA_KIND_UINT16",
            "SSZ_STATIC_SCHEMA_KIND_UINT32",
            "SSZ_STATIC_SCHEMA_KIND_UINT64",
            "SSZ_STATIC_SCHEMA_KIND_UINT128",
            "SSZ_STATIC_SCHEMA_KIND_UINT256",
        ):
            size_by_kind = {
                "SSZ_STATIC_SCHEMA_KIND_BOOL": 1,
                "SSZ_STATIC_SCHEMA_KIND_UINT8": 1,
                "SSZ_STATIC_SCHEMA_KIND_UINT16": 2,
                "SSZ_STATIC_SCHEMA_KIND_UINT32": 4,
                "SSZ_STATIC_SCHEMA_KIND_UINT64": 8,
                "SSZ_STATIC_SCHEMA_KIND_UINT128": 16,
                "SSZ_STATIC_SCHEMA_KIND_UINT256": 32,
            }
            fixed_size = size_by_kind[kind]
            min_size = fixed_size
        elif kind == "SSZ_STATIC_SCHEMA_KIND_BYTE_VECTOR":
            param = int(signature[1])
            fixed_size = param
            min_size = param
        elif kind == "SSZ_STATIC_SCHEMA_KIND_BYTE_LIST":
            param = int(signature[1])
            fixed_size = None
            min_size = 0
        elif kind == "SSZ_STATIC_SCHEMA_KIND_BIT_VECTOR":
            param = int(signature[1])
            fixed_size = self._bits_to_bytes(param)
            min_size = fixed_size
        elif kind == "SSZ_STATIC_SCHEMA_KIND_BIT_LIST":
            param = int(signature[1])
            fixed_size = None
            min_size = 1
        elif kind in ("SSZ_STATIC_SCHEMA_KIND_VECTOR", "SSZ_STATIC_SCHEMA_KIND_LIST"):
            param = int(signature[1])
            child_schema = schema_type.element_cls()
            child_type_index = self._intern(child_schema)
            child_entry = self.types[child_type_index]

            if kind == "SSZ_STATIC_SCHEMA_KIND_VECTOR":
                if child_entry.fixed_size is not None:
                    fixed_size = child_entry.fixed_size * param
                else:
                    fixed_size = None
                min_size = child_entry.min_size * param
            else:
                fixed_size = None
                min_size = 0
        elif kind == "SSZ_STATIC_SCHEMA_KIND_CONTAINER":
            field_types = list(schema_type.fields().values())
            field_count = len(field_types)
            total_fixed = 0
            total_min = 0
            pending_fields: List[FieldEntry] = []

            for field_type in field_types:
                child_index = self._intern(field_type)
                child_entry = self.types[child_index]
                pending_fields.append(
                    FieldEntry(
                        child_type_index=child_index,
                        fixed_size=child_entry.fixed_size,
                    )
                )
                if child_entry.fixed_size is None:
                    total_fixed = -1
                    total_min += 4 + child_entry.min_size
                else:
                    if total_fixed >= 0:
                        total_fixed += child_entry.fixed_size
                    total_min += child_entry.fixed_size

            fixed_size = None if total_fixed < 0 else total_fixed
            min_size = total_min
            field_index = len(self.fields)
            self.fields.extend(pending_fields)
        else:
            raise RuntimeError(f"unknown schema kind {kind}")

        type_index = len(self.types)
        self._signature_to_index[signature] = type_index
        self.types.append(
            TypeEntry(
                kind=kind,
                param=param,
                child_type_index=child_type_index,
                field_index=field_index,
                field_count=field_count,
                fixed_size=fixed_size,
                min_size=min_size,
            )
        )
        return type_index

    def build_registry(self, entries: Sequence[RegistryEntry]) -> List[Tuple[RegistryEntry, int]]:
        registry: List[Tuple[RegistryEntry, int]] = []

        for entry in entries:
            schema_type = import_schema_type(entry)
            type_index = self._intern(schema_type)
            registry.append((entry, type_index))

        return registry


def render_header() -> str:
    return f"""/*
 * Auto-generated by tests/spec/generate_ssz_static_schema.py.
 *
 * The generator discovers ssz_static handlers from the fixture tree and maps
 * each (preset, fork, handler) tuple to a deduplicated structural schema. The
 * runtime can interpret these generic tables without any hand-written beacon
 * descriptors. Union schemas are intentionally unsupported because the current
 * fixture-discovered handler set contains none; the generator fails if that
 * changes.
 */
#ifndef {SCHEMA_HEADER_GUARD}
#define {SCHEMA_HEADER_GUARD}

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {{
#endif

#define SSZ_STATIC_SCHEMA_DYNAMIC_SIZE ((size_t)SIZE_MAX)
#define SSZ_STATIC_SCHEMA_NO_CHILD_INDEX UINT32_MAX

typedef enum
{{
    SSZ_STATIC_SCHEMA_KIND_BOOL = 0,
    SSZ_STATIC_SCHEMA_KIND_UINT8,
    SSZ_STATIC_SCHEMA_KIND_UINT16,
    SSZ_STATIC_SCHEMA_KIND_UINT32,
    SSZ_STATIC_SCHEMA_KIND_UINT64,
    SSZ_STATIC_SCHEMA_KIND_UINT128,
    SSZ_STATIC_SCHEMA_KIND_UINT256,
    SSZ_STATIC_SCHEMA_KIND_BYTE_VECTOR,
    SSZ_STATIC_SCHEMA_KIND_BYTE_LIST,
    SSZ_STATIC_SCHEMA_KIND_BIT_VECTOR,
    SSZ_STATIC_SCHEMA_KIND_BIT_LIST,
    SSZ_STATIC_SCHEMA_KIND_VECTOR,
    SSZ_STATIC_SCHEMA_KIND_LIST,
    SSZ_STATIC_SCHEMA_KIND_CONTAINER
}} ssz_static_schema_kind_t;

typedef struct
{{
    uint32_t child_type_index;
    size_t fixed_size;
}} ssz_static_schema_field_t;

typedef struct
{{
    ssz_static_schema_kind_t kind;
    uint64_t param;
    uint32_t child_type_index;
    uint32_t field_index;
    uint32_t field_count;
    size_t fixed_size;
    size_t min_size;
}} ssz_static_schema_type_t;

typedef struct
{{
    const char *preset;
    const char *fork;
    const char *handler;
    uint32_t type_index;
}} ssz_static_schema_registry_entry_t;

extern const ssz_static_schema_field_t g_ssz_static_schema_fields[];
extern const size_t g_ssz_static_schema_field_count;
extern const ssz_static_schema_type_t g_ssz_static_schema_types[];
extern const size_t g_ssz_static_schema_type_count;
extern const ssz_static_schema_registry_entry_t g_ssz_static_schema_registry[];
extern const size_t g_ssz_static_schema_registry_count;

#ifdef __cplusplus
}}
#endif

#endif
"""


def render_source(
    types: Sequence[TypeEntry],
    fields: Sequence[FieldEntry],
    registry: Sequence[Tuple[RegistryEntry, int]],
) -> str:
    field_lines = []
    for field in fields:
        field_lines.append(
            "    {"
            f" {field.child_type_index}u, {format_size(field.fixed_size)} "
            "},"
        )

    type_lines = []
    for type_entry in types:
        child_index = (
            f"{type_entry.child_type_index}u"
            if type_entry.child_type_index is not None
            else NO_CHILD_INDEX
        )
        type_lines.append(
            "    {"
            f" {type_entry.kind}, {format_u64(type_entry.param)}, {child_index},"
            f" {type_entry.field_index}u, {type_entry.field_count}u,"
            f" {format_size(type_entry.fixed_size)}, {format_size(type_entry.min_size)} "
            "},"
        )

    registry_lines = []
    for entry, type_index in registry:
        registry_lines.append(
            "    {"
            f' "{entry.preset}", "{entry.fork}", "{entry.handler}", {type_index}u '
            "},"
        )

    return (
        """/*
 * Auto-generated by tests/spec/generate_ssz_static_schema.py.
 * See the paired header for design notes.
 */
#include "ssz_static_schema_generated.h"

const ssz_static_schema_field_t g_ssz_static_schema_fields[] = {
"""
        + ("\n".join(field_lines) if field_lines else "    { 0u, ((size_t)0u) },")
        + """
};
const size_t g_ssz_static_schema_field_count =
    sizeof(g_ssz_static_schema_fields) / sizeof(g_ssz_static_schema_fields[0]);

const ssz_static_schema_type_t g_ssz_static_schema_types[] = {
"""
        + ("\n".join(type_lines) if type_lines else "    { SSZ_STATIC_SCHEMA_KIND_BOOL, UINT64_C(0), SSZ_STATIC_SCHEMA_NO_CHILD_INDEX, 0u, 0u, (size_t)1u, (size_t)1u },")
        + """
};
const size_t g_ssz_static_schema_type_count =
    sizeof(g_ssz_static_schema_types) / sizeof(g_ssz_static_schema_types[0]);

const ssz_static_schema_registry_entry_t g_ssz_static_schema_registry[] = {
"""
        + ("\n".join(registry_lines) if registry_lines else '    { "", "", "", 0u },')
        + """
};
const size_t g_ssz_static_schema_registry_count =
    sizeof(g_ssz_static_schema_registry) / sizeof(g_ssz_static_schema_registry[0]);
"""
    )


def write_if_changed(path: Path, content: str) -> None:
    path.parent.mkdir(parents=True, exist_ok=True)
    if path.exists() and path.read_text(encoding="utf-8") == content:
        return
    path.write_text(content, encoding="utf-8")


def main() -> int:
    args = parse_args()
    ensure_pyspec_root(args.pyspec_root)

    entries = discover_registry_entries(args.fixtures_root)
    generator = SchemaGenerator()
    registry = generator.build_registry(entries)

    write_if_changed(args.output_header, render_header())
    write_if_changed(args.output_source, render_source(generator.types, generator.fields, registry))
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
