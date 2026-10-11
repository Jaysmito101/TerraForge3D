# API schema source

This directory contains the operation metadata and JSON schemas used by the
TerraForge3D API. The application composes these files at startup. The composed
schema is shared by request validation, the OpenAPI document, and generated
operation skill pages.

## Layout

```text
Data/Api/Schemas/
├── Common/                         reusable fragments and operation metadata
└── Operations/<Domain>/            request and state schemas
    └── Definitions/                operation id, text, schema, and flags
```

Reusable schemas belong in `Common/` or the relevant operation domain. A
definition normally includes `Common/Operation.json`:

```json
{
  "$include": "Common/Operation.json",
  "Name": "tf3d.viewport.update_state",
  "Method": "PATCH",
  "Path": "/api/v1/viewports/{ViewportId}",
  "Title": "Update viewport state",
  "Description": "Update writable viewport state.",
  "InputSchema": { "$include": "Operations/Viewport/Update.json" },
  "Annotations": { "readOnlyHint": false },
  "Flags": ["None"]
}
```

`Name`, `Method`, `Path`, `Title`, `Description`, and `InputSchema` are
required. `Method` is `GET`, `POST`, or `PATCH`. `Path` starts with
`/api/v1/`; path placeholders use `{Name}` and must refer to required
properties in `InputSchema`. Operation IDs also define the documentation
hierarchy: `tf3d.renderer.terrain.get_state` is exposed under
`renderer/terrain/get-state`. `Annotations` must be an object when present.
Supported flags are `None`, `ReadOnly`, `Destructive`, and `LongRunning`.

## Composing schemas

`$include` accepts one path or an array of paths:

```json
{
  "$include": "Common/Object.json",
  "properties": {
    "Position": { "$include": "Common/Vector3.json", "readOnly": true }
  }
}
```

- A bare path is relative to `Data/Api/Schemas`.
- A path beginning with `./` or `../` is relative to the including file.
- Includes can be nested at object or array nodes.
- Included objects merge recursively in order; later and local fields win.
- Arrays and scalar values replace earlier values.
- Paths cannot escape the schema root. Missing files, invalid JSON, and
  include cycles fail composition with an include-chain error.

`$runtime` inserts a value supplied by a registered C++ provider:

```json
{
  "type": "string",
  "enum": { "$runtime": "Viewport.Modes" }
}
```

Current providers supply viewport modes and texture indices, plus runtime
renderer state for terrain, lights, sky, and sea. An unavailable provider value
fails composition.

## Request validation

The API assembles arguments from path parameters, GET query parameters, or a
POST/PATCH JSON body, then validates the complete argument object against the
composed schema before calling an operation handler. Query and path values are
converted using their declared scalar types. The validator currently handles:

- `type`, `enum`, and `const`
- `required`, `properties`, and boolean or schema-valued
  `additionalProperties`
- `items`, `minItems`, and `maxItems`
- `minLength`, `maxLength`, and `pattern`
- `minimum`, `maximum`, `exclusiveMinimum`, and `exclusiveMaximum`
- `allOf`, `anyOf`, and `oneOf`

Operation handlers still perform application-specific checks, such as
confirming that an object exists or that a renderer accepts a selected mode.
Update handlers also apply the composed read-only policy and reject supplied
fields marked `readOnly`.

## Example: partial viewport update

The `tf3d.viewport.update_state` definition includes
`Operations/Viewport/Update.json`. That schema requires `ViewportId` and
`State`; `State` describes only writable fields, so callers can send a partial
update:

```json
{
  "ViewportId": 1,
  "State": {
    "Camera": {
      "Target": { "X": 0.0, "Y": 20.0, "Z": 0.0 },
      "Distance": 100.0
    }
  }
}
```

The compact directory at `GET /api/v1/operations` links to one operation at a
time. Its details link to the composed JSON Schema at
`GET /api/v1/operations/viewport/update-state/schema` and the generated skill
at `GET /api/v1/skills/viewport/update-state.md`. The schema endpoint returns
the full validated argument schema with its `$schema` dialect and a relative
`$id` resolved from the schema URL. This keeps the schema usable under the
public host and any proxy path prefix. The OpenAPI document also describes the
HTTP body and parameter locations. Keep the composed schema as the input
contract and the C++ handler as the authority for domain behavior.

For this update, `ViewportId` is supplied in the URL and only `State` goes in
the JSON body:

```http
PATCH /api/v1/viewports/1
Content-Type: application/json

{"State":{"Camera":{"Distance":100.0}}}
```
