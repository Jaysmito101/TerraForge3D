# MCP JSON Schema templates

This directory is the source for MCP tool metadata and input schemas. Files are
composed at startup from `Data/Mcp/Schemas` and the resolved JSON is what MCP
receives. `$include` and `$runtime` are authoring directives; neither remains in
the registered schema.

## Schema body

Use ordinary JSON Schema keywords in lowercase:

```json
{
  "type": "object",
  "additionalProperties": false,
  "properties": {
    "ViewportId": { "type": "integer", "minimum": 0 },
    "Enabled": { "type": "boolean" }
  },
  "required": ["ViewportId"]
}
```

Common keywords are `type`, `properties`, `required`, `additionalProperties`,
`items`, `enum`, `minimum`, `maximum`, `exclusiveMinimum`, `minItems`,
`maxItems`, `minLength`, and `readOnly`. Application fields use PascalCase
(`ViewportId`, `PositionOnTerrain`, `X`, `Y`, `Z`); JSON Schema keywords stay
lowercase.

## `$include`

`$include` accepts one path or an array of paths:

```json
{
  "$include": "Common/Object.json",
  "properties": {
    "Position": { "$include": "Common/Vector3.json", "readOnly": true }
  }
}
```

- A bare path is relative to `Data/Mcp/Schemas`.
- A path beginning with `./` or `../` is relative to the file containing it.
- Includes are recursive and may appear at any object or array node.
- Included objects are deep-merged in order; later includes and local fields win.
  Object members merge recursively, while arrays and scalar values replace.
- An included array used as an array item is spliced into the containing array.
- Multiple includes at one object node must resolve to objects.
- Paths are kept inside the schema root. Missing files, invalid JSON, and cycles
  fail composition with an include-chain error.

Keep reusable fragments in `Common/` or the relevant tool subtree. The usual
pattern is a small base object plus local `properties` and `required` fields:

```json
{
  "$include": "Common/Object.json",
  "properties": {
    "State": { "$include": "Tools/Viewport/State.json" }
  },
  "required": ["State"]
}
```

## `$runtime`

`$runtime` asks a named C++ provider for a JSON value at composition time:

```json
{
  "type": "string",
  "enum": { "$runtime": "Viewport.Modes" }
}
```

The provider name must be a string and must be registered by the corresponding
tool layer. Current examples include `Viewport.Modes`,
`Viewport.TextureSlotIndices`, `Viewport.TextureChannelCount`, `Sea.State`,
`Terrain.State`, `Sky.State`, and `Lights.State`. If a runtime value is
unavailable, composition fails. A runtime value may be extended with local
object fields only when the returned value is an object.

## Tool and action layout

Reusable state/input schemas live under `Tools/<Domain>/`. Action definitions
live under `Tools/<Domain>/Actions/` and normally include `Common/Action.json`:

```json
{
  "$include": "Common/Action.json",
  "Name": "tf3d.viewport.update_state",
  "Title": "Update viewport state",
  "Description": "Update writable viewport state.",
  "InputSchema": { "$include": "Tools/Viewport/Update.json" },
  "Annotations": { "readOnlyHint": false },
  "Flags": ["None"]
}
```

`Name`, `Title`, `Description`, and `InputSchema` are required by the action
loader. `Annotations` must be an object when present. Valid `Flags` are
`None`, `ReadOnly`, `Destructive`, and `LongRunning`.

Keep canonical state definitions separate from update policies. For example,
`State.json` describes the complete state, `Update.json` describes writable
input, and `UpdateStateReadOnly.json` can be used to reject fields that are
returned but must not be written.

## Read-only update validation

Update handlers may pass the composed schema to
`McpSchemaTemplate::ValidateWritable`. It recursively visits supplied object
properties and array items and rejects a supplied field whose schema contains
`"readOnly": true`. This is a writable-field guard, not a replacement for a
full JSON Schema validator; keep normal type/range validation in the schema and
domain validation in the handler.
