# MCP schema templates

Schemas are composed at startup from the JSON tree below `Data/Mcp/Schemas`.

`$include` composes another JSON object. Paths without `./` or `../` are relative
to the schema root, while `./` and `../` paths are relative to the current file.
Object includes are deep-merged, so a common object/vector template can be
extended with local properties.

`$runtime` replaces the directive with a typed JSON value supplied by C++:

```json
{ "enum": { "$runtime": "Viewport.Modes" } }
```

The final document sent to MCP contains only standard JSON Schema keywords;
composition and runtime directives are removed before registration.

Application data fields use PascalCase, including vector components such as
`X`, `Y`, and `Z`. JSON Schema keywords remain lowercase as required by the
JSON Schema specification.

Tool metadata is also template-driven. Action templates under `Tools/**/Actions`
define `Name`, `Title`, `Description`, `InputSchema`, `Annotations`, and
`Flags`; C++ only supplies the invocation handler.

Update handlers can pass a composed schema to `McpSchemaTemplate::ValidateWritable`.
The validator walks nested objects and arrays and rejects any supplied field whose
schema has `readOnly: true`, so read-only update rules remain in the JSON schema
tree instead of being duplicated in tool code. Viewport update policies compose
the canonical state templates, so writable camera controls and read-only camera
projection fields stay defined in one place.
