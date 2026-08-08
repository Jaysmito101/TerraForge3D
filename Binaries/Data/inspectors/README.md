# Custom Inspector JSON schema

Files in this directory define the values, widgets, layout, and optional
external schema metadata for `CustomInspector`. A file named
`Data/inspectors/<Name>.json` is loaded by the application and may be reused by
generators, filters, and renderer settings.

## Top-level document

```json
{
  "Description": "Short help text for the inspector.",
  "ShowResetButton": true,
  "Schema": { "properties": {} },
  "Params": [],
  "Sections": [],
  "Buttons": [],
  "WidgetOrder": [],
  "Presets": []
}
```

All fields are optional. `ShowResetButton` defaults to `true`. `Params` and
`Buttons` may be placed at the root or inside a section. `ID` and other
metadata may be consumed by the owning generator, but are not layout controls
interpreted by `CustomInspector` itself.

### Sections

`Sections` is an ordered array. Each section supports:

| Field | Type | Meaning |
| --- | --- | --- |
| `Name` | string | Stable section identifier; defaults to `Section`. |
| `Label` | string | Visible heading; defaults to `Name`. |
| `Description` | string | Section help text. |
| `Collapsible` | bool | Whether the section can be collapsed. |
| `DefaultOpen` | bool | Initial open state; defaults to `true`. |
| `Params` | array | Values/widgets in the section. |
| `Buttons` | array | Actions displayed in the section. |

## Parameters

Every parameter creates one stored value and, unless hidden, one widget:

```json
{
  "Name": "Strength",
  "SerializedName": "Strength",
  "Type": "Float",
  "Default": 1.0,
  "Widget": "Slider",
  "Constraints": [0.0, 2.0],
  "Sensitivity": 0.01,
  "Label": "Strength",
  "Description": "Scales the effect.",
  "Tooltip": "Displayed when hovering the control.",
  "ShaderUniform": "u_Strength",
  "Visible": true,
  "Conditions": [{ "Name": "Mode", "Values": [0, 2] }]
}
```

| Field | Type | Meaning |
| --- | --- | --- |
| `Name` | string | Runtime value key and default widget variable name. |
| `SerializedName` | string | Optional save/schema key; defaults to `Name`. |
| `Type` | string | Stored value type; see the type table below. Defaults to `Float`. |
| `Default` | type-dependent | Initial value. If omitted, a zero/empty value is used. |
| `Widget` | string | UI widget; defaults to `Input`. |
| `Label` | string | Visible label; defaults to `Name`. |
| `Description` | string | Fallback tooltip/help text. |
| `Tooltip` | string | Tooltip; takes precedence over `Description`. |
| `Visible` | bool | `false` creates the value but hides its widget. |
| `Constraints` | number array | `[min, max]`, optionally `[c, d]`; numeric widgets use the first pair. |
| `Sensitivity` | number | Drag/slider/seed step size. |
| `Options` | string array | Dropdown labels. |
| `OptionValues` | integer array | Optional dropdown values, in the same order as `Options`. |
| `ShaderUniform` | string | Explicit shader uniform. An empty string disables binding. |
| `Conditions` | object array | Multiple visibility conditions. |
| `Conditional` | string | Legacy single-condition form. |
| `ConditionalValue(s)` | scalar/array | Legacy value(s) for `Conditional`. |
| `Count` | integer | `FloatArray` length when no default array is supplied. |
| `PointCount` | integer | Initial point count for `Path` or `Curve`. |
| `BitDepth` | integer | Texture default loading uses 16-bit mode when `>= 16`. |

### Value types

| `Type` | JSON default/state form | Notes |
| --- | --- | --- |
| `Int` | integer | Integer value. |
| `Float` | number | Floating-point value. |
| `Bool` | boolean | Checkbox-compatible value. |
| `String` | string | Text or path-like value. |
| `Vector2` | `[x, y]` | Two-component vector. |
| `Vector3` | `[x, y, z]` | Three-component vector. |
| `Vector4` | `[x, y, z, w]` | Four-component vector. |
| `FloatArray` | number array | Fixed to the configured/default length. |
| `Texture` | string path or empty | Loaded texture reference. |
| `Path` | `[[x, y], ...]` | Up to 16 points; at least one point. |
| `Curve` | `[[x, y], ...]` | Up to 16 points; at least two points. |

### Widget types

Supported names are `Slider`, `Drag`, `Color`, `Texture`, `Path`, `Curve`,
`Octaves`, `Button`, `Checkbox`, `Input`, `Seed`, `Dropdown`, `Separator`,
`NewLine`, and `Text`. `Separator`, `NewLine`, and `Text` are layout/display
widgets; value widgets should normally match the parameter type. `Dropdown`
uses `Options` as labels and `OptionValues` as the stored integer values.
`Octaves` is the array-oriented control used by noise settings.

By default shader binding uses `u_` plus `Name`. Use `ShaderUniform` when the
uniform differs, or `"ShaderUniform": ""` to disable it. `Path` additionally
publishes a `<uniform>Count` integer when bound.

## Conditions

Conditions compare another integer-like value. Each condition passes when its
current value matches one of the listed values; when multiple conditions are
present, all conditions must pass before the widget is shown:

```json
"Conditions": [
  { "Name": "MaskType", "Values": [2, 20] },
  { "Name": "NoiseAlgorithm", "Values": [1, 3] }
]
```

The `Values` form is preferred. The legacy equivalent is:

```json
"Conditional": "MaskType",
"ConditionalValues": [2, 20]
```

Use integer values from the controlling parameter. For option-backed values,
catalog/metadata preprocessing may translate symbolic names to those integers
before the inspector is loaded. Conditions are UI visibility rules; they do not
remove the value from saved state or shader metadata.

## Buttons, order, presets, and custom schema

Buttons use `Action`, optional `Label`, and optional `Description`/`Tooltip`:

```json
"Buttons": [
  { "Action": "ResetRecommended", "Label": "Reset recommended" }
]
```

`WidgetOrder` is an array of widget labels or parameter names. Unlisted widgets
are appended in their existing order. `Presets` contains named value sets:

```json
"Presets": [
  {
    "Name": "Soft",
    "Label": "Soft response",
    "Description": "A gentler preset.",
    "Values": { "Strength": 0.5 }
  }
]
```

`Name` is required and must not be `Default`; `Values` must be an object and
may use either runtime names or `SerializedName` keys. Invalid or duplicate
presets are skipped. The built-in `Default` entry always means reset to the
declared defaults.

`Schema` is optional JSON Schema metadata merged into the schema generated from
the inspector values. Use it for read-only fields, titles/descriptions, or
domain-specific constraints that the generic value/widget mapping cannot
express. It does not create an inspector control by itself.

`BuildSchema()` emits an object schema: root-level parameters become root
properties, section parameters are nested under their section `Name`, vectors
become objects with required `X`/`Y`/`Z`/`W` fields, dropdowns emit `enum` and
`x-enumNames`, and numeric constraints become `minimum`/`maximum`. The custom
`Schema` object is deep-merged over that generated schema, so it can add or
refine fields without duplicating the inspector definition.

## Includes

Inspector files support the same `$include` directive as MCP schemas:

```json
{
  "$include": "Common/NoiseParams.json",
  "Sections": [
    { "$include": "Sections/Blend.json" }
  ]
}
```

For inspector metadata, paths are relative to the file containing the include.
Included objects are recursively merged, local fields override included fields,
and an included array used as an array item is flattened. Missing files,
invalid JSON, and cycles are errors. Resolve includes before relying on
`Sections`, `Params`, or `WidgetOrder` structure.
