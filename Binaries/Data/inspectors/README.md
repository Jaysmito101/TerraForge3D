# Custom Inspector JSON schema

Files in this directory define the values, widgets, layout, and optional
external schema metadata for `CustomInspector`. Each inspector has a stable
folder entrypoint at `Data/inspectors/<Name>/Inspector.json`; fragments live
beside that entrypoint and may be reused by generators, filters, and renderer
settings.

The current layout is intentionally domain-oriented:

```text
inspectors/
  Common/
    Noise.json
  BaseNoise/
    Inspector.json
    Noise.json
    Octaves.json
    Blend.json
  CalculatedMask/
    Inspector.json
    Mask.json
    Algorithms/
      HeightRange.json
      SlopeRange.json
      ...
    Parameters/
      Common/
        Range.json
        EdgeFeather.json
      Terrain/
      Spatial/
      Pattern/
      Noise.json
  Filters/
    Inspector.json
    Denoise/
      BoxBlur.json
      GaussianSmooth.json
      ...
    Shaping/
      Curve.json
      Flatten.json
      ...
```

Keep `Inspector.json` as the only runtime entrypoint. Include section objects
inside `Sections` and parameter arrays inside `Params`; do not nest inspector
sections inside other sections.

`Filters/Inspector.json` is a catalog entrypoint. It includes one complete
filter definition per `Sections` item. A filter definition separates its
runtime execution description from the UI metadata in `Runtime` and
`Inspector`; the generic `CustomInspector` only consumes the latter.

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
| `CustomData` | JSON value | Opaque owner metadata; `CustomInspector` stores it but does not interpret it. |

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

## Filter metadata and execution

Filters are data-defined GPU operations. The filter catalog registers their
identity, shader phases, execution graph, and inspector controls. The generic
filter executor then runs the same pipeline for every definition, so adding a
normal filter does not require a new C++ class or a new dispatch branch.

The files are split by responsibility:

```text
Data/
  inspectors/
    Filters/
      Inspector.json                 # filter catalog entrypoint
      Denoise/BoxBlur.json            # one filter definition
      Shaping/Curve.json
  shaders/
    generation/filters/
      common/                         # shared field and merge shaders
      denoise/box_blur/blur.glsl      # filter phase shader
      shaping/curve/curve.glsl
```

The catalog includes definitions explicitly:

```json
{
  "ID": "filters",
  "Name": "Filter catalog",
  "ShowResetButton": false,
  "Sections": [
    { "$include": "./Denoise/BoxBlur.json" },
    { "$include": "./Shaping/Curve.json" }
  ]
}
```

The include path is relative to `Filters/Inspector.json`. The filter catalog
resolves these entries before constructing `BiomeFilterDefinition` objects.
The filter `ID` is the stable saved-project identifier; change its display
`Name` freely, but do not rename `ID` after users have saved projects with it.

### Filter definition schema

This is the complete shape of a phase-chain filter. Optional objects can be
omitted when they have no entries, but a usable filter needs at least one
operation pass, a merge phase, and the corresponding shader paths.

```json
{
  "ID": "box_blur",
  "Name": "Box Blur",
  "Category": "Denoise",
  "Description": "Short help text shown above the filter controls.",

  "Runtime": {
    "Implementation": "PhaseChain",
    "Defaults": {
      "MergeMode": "Blend"
    },
    "Statistics": {
      "NeedsMinMax": false,
      "NeedsHistogram": false,
      "RequestedPercentileParameter": "Section.Parameter"
    },
    "Shaders": {
      "Phases": {
        "Operation": "generation/filters/denoise/box_blur/blur",
        "Merge": "generation/filters/common/merge"
      }
    },
    "Resources": {
      "Temps": [
        { "Name": "Operation", "Type": "Field" }
      ]
    },
    "Execution": {
      "Passes": [
        {
          "Phase": "Operation",
          "Input": "Current",
          "Output": "Operation",
          "Uniforms": {}
        }
      ],
      "Merge": {
        "Phase": "Merge",
        "Input": "Current",
        "Operation": "Operation",
        "Output": "Next"
      }
    }
  },

  "Inspector": {
    "ShowResetButton": true,
    "Sections": [
      {
        "Name": "Settings",
        "Label": "Settings",
        "Params": []
      }
    ]
  }
}
```

#### Fixed runtime metadata

These fields are interpreted by the C++ filter runtime and have a fixed
meaning:

| Path | Type | Meaning |
| --- | --- | --- |
| `Runtime.Implementation` | enum string | Currently only `PhaseChain` is supported. A new backend requires C++ changes. |
| `Runtime.Defaults.MergeMode` | enum string | `Override`, `Add`, `Subtract`, `Multiply`, or `Blend`; defaults to `Blend`. |
| `Runtime.Statistics.NeedsMinMax` | bool | Requests field min/max statistics before execution. |
| `Runtime.Statistics.NeedsHistogram` | bool | Requests histogram statistics before execution. |
| `Runtime.Statistics.RequestedPercentileParameter` | string | Inspector path whose value supplies the requested percentile, for example `Level.Target`. |
| `Runtime.Shaders.Phases` | object | Maps logical phase names to shader paths. |
| `Runtime.Resources.Temps[].Type` | enum string | Currently `Field`; each entry declares a reusable temporary field. |

Shader paths are relative to `Data/shaders` and normally omit the `.glsl`
extension. Phase names are logical identifiers: every `Execution` pass must
refer to a name declared in `Shaders.Phases`.

`Resources.Temps` names are local to the filter execution graph. The executor
also provides these built-in resources without declaring them:

| Resource | Meaning |
| --- | --- |
| `Current` | The field entering this filter. |
| `Next` | The filter's output field. |
| `Output` | Alias for the output field used by pass validation. |
| `IterationResult` | Result of a ping-pong iteration; available only with `IterationMode: "PingPong"`. |

#### Execution metadata

`Execution.Passes` is the main operation sequence. A pass has this shape:

```json
{
  "Phase": "Operation",
  "Input": "Current",
  "Output": "Operation",
  "Reference": "Original",
  "Uniforms": {
    "u_Radius": {
      "Parameter": "Settings.Radius"
    },
    "u_Axis": {
      "Value": 0,
      "Type": "Int"
    }
  }
}
```

`Input`, `Output`, and optional `Reference` must name built-in or declared
resources. `Reference` is useful for passes that need the original field while
another field is being iterated. `PostPasses` has the same pass format and runs
after the main passes or after a ping-pong loop.

The optional iteration fields are:

| Field | Meaning |
| --- | --- |
| `IterationMode` | Set to `PingPong` to repeat one operation pass between two temporary fields. |
| `IterationBuffers` | Exactly two different resource names used alternately by the loop. |
| `IterationsParameter` | Inspector path containing the iteration count. The executor clamps it to `[0, 64]`. |
| `UseOriginalInput` | For ping-pong filters, use `Current` as the fallback reference for every iteration. |
| `Setup` | Optional pass that initializes the first iteration buffer. |
| `PostPasses` | Optional passes applied after the main operation. |

Ping-pong execution currently supports exactly one operation pass. A typical
multi-pass filter such as a separable blur instead uses several ordinary
`Passes`, with each pass writing to the next logical temporary resource.

`Execution.Merge` is required for every filter:

```json
{
  "Phase": "Merge",
  "Input": "Current",
  "Operation": "Operation",
  "Output": "Next",
  "Uniforms": {
    "u_InvalidOperationThreshold": {
      "Value": 1000000.0,
      "Type": "Float"
    }
  }
}
```

The common merge shader combines the original input and operation result using
the filter's C++-owned `Strength`, `MergeMode`, and optional mask. A strength
of zero is an identity operation. The common UI controls `Enabled`,
`Strength`, `Merge mode`, `Use mask`, and `Invert mask` are owned by
`BiomeFilter`; do not duplicate them inside `Inspector.Sections`.

### Shader contract

Operation shaders should include the shared field helpers:

```glsl
#version 430 core

layout(local_size_x = 16, local_size_y = 16, local_size_z = 1) in;

#include "../../common/field_common.glsl"

uniform float u_Amount;

void main()
{
    ivec2 coordinate = ivec2(gl_GlobalInvocationID.xy);
    if (coordinate.x >= u_Resolution || coordinate.y >= u_Resolution) {
        return;
    }

    float inputValue = sampleInput(coordinate);
    writeOutput(coordinate, mix(inputValue, 1.0f - inputValue, u_Amount));
}
```

`field_common.glsl` provides the standard image bindings and helpers:

| Binding/uniform | Meaning |
| --- | --- |
| `InputData`, binding `0` | Read-only input field. |
| `ReferenceData`, binding `1` | Optional reference field when a pass declares `Reference`. |
| `OutputData`, binding `2` | Write-only operation output. |
| `u_Resolution` | Working field width and height. |
| `FieldStatisticsBuffer`, binding `4` | Optional statistics buffer when `Statistics` requests it. |

The executor dispatches every phase over the working square and inserts a
memory barrier between phases. The common merge phase uses input binding `0`,
operation binding `1`, output binding `2`, and the optional mask at binding
`3`. Custom operation shaders should write every in-bounds pixel with
`writeOutput`.

### Uniform bindings

`Execution.*.Uniforms` maps shader uniform names to either inspector values or
constants. Parameter paths use the inspector section name followed by the
parameter name:

```json
"Uniforms": {
  "u_Radius": { "Parameter": "Smoothing.Radius" },
  "u_Enabled": { "Parameter": "Settings.Enabled" },
  "u_Axis": { "Value": 0, "Type": "Int" },
  "u_Color": { "Value": [1.0, 0.5, 0.0], "Type": "Vector3" }
}
```

Parameter bindings use the stored `CustomInspector` value type. Integers,
floats, booleans, and vectors are uploaded directly. Texture parameters bind
the texture and can optionally set a presence uniform:

```json
"u_Heightmap": {
  "Parameter": "Source.Heightmap",
  "PresenceUniform": "u_HasHeightmap"
}
```

Curve parameters upload the control-point array and a point-count uniform. Use
`PointCountUniform` when the shader uses a name other than the default
`<uniform>PointCount`:

```json
"u_Curve": {
  "Parameter": "Curve.Curve",
  "PointCountUniform": "u_CurvePointCount"
}
```

The supported inspector parameter types and widgets are documented above in
the generic schema. The parameter must exist in `Inspector.Sections`; a typo
in a `Parameter` path silently leaves that uniform unset, so keep paths close
to the corresponding section and verify them when adding a filter.

### Adding a new filter

For a normal phase-chain filter, use this workflow:

1. Create a shader directory under
   `Data/shaders/generation/filters/<category>/<id>/`.
2. Add one or more compute phase shaders. Include `field_common.glsl`, use the
   standard bindings, and write the operation result to `OutputData`.
3. Create a definition under
   `Data/inspectors/Filters/<Category>/<Name>.json` using the `Runtime` and
   `Inspector` schema above.
4. Add the definition to `Data/inspectors/Filters/Inspector.json` with a
   relative `$include` entry.
5. Add inspector parameters under `Inspector.Sections` and bind them from
   execution passes using paths such as `Settings.Radius`.
6. Make sure every phase name, resource name, input, output, and parameter path
   matches exactly.
7. Build and start the application. The startup log should report the updated
   filter-definition count; then exercise the filter with strength zero, full
   strength, a mask, and another filter before and after it in the stack.

Here is a small complete definition for a value-inverting operation. It needs
one shader at `generation/filters/shaping/invert/invert.glsl`:

```json
{
  "ID": "invert",
  "Name": "Invert Field",
  "Category": "Shaping",
  "Description": "Moves the operation field toward its inverted value.",
  "Runtime": {
    "Implementation": "PhaseChain",
    "Shaders": {
      "Phases": {
        "Invert": "generation/filters/shaping/invert/invert",
        "Merge": "generation/filters/common/merge"
      }
    },
    "Resources": {
      "Temps": [
        { "Name": "Inverted", "Type": "Field" }
      ]
    },
    "Execution": {
      "Passes": [
        {
          "Phase": "Invert",
          "Input": "Current",
          "Output": "Inverted",
          "Uniforms": {
            "u_Amount": { "Parameter": "Settings.Amount" }
          }
        }
      ],
      "Merge": {
        "Phase": "Merge",
        "Input": "Current",
        "Operation": "Inverted",
        "Output": "Next"
      }
    }
  },
  "Inspector": {
    "ShowResetButton": true,
    "Sections": [
      {
        "Name": "Settings",
        "Label": "Settings",
        "Params": [
          {
            "Name": "Amount",
            "Type": "Float",
            "Default": 1.0,
            "Widget": "Slider",
            "Constraints": [0.0, 1.0],
            "Description": "Amount of inversion in the operation field."
          }
        ]
      }
    ]
  }
}
```

The corresponding shader is:

```glsl
#version 430 core

layout(local_size_x = 16, local_size_y = 16, local_size_z = 1) in;
#include "../../common/field_common.glsl"

uniform float u_Amount;

void main()
{
    ivec2 coordinate = ivec2(gl_GlobalInvocationID.xy);
    if (coordinate.x >= u_Resolution || coordinate.y >= u_Resolution) {
        return;
    }

    float inputValue = sampleInput(coordinate);
    writeOutput(coordinate, mix(inputValue, 1.0f - inputValue, u_Amount));
}
```

Finally add it to the catalog:

```json
{
  "$include": "./Shaping/Invert.json"
}
```

No filter-specific C++ is needed for this example. C++ changes are required
only when introducing a new `Runtime.Implementation`, a new resource/pass
semantic, or a new inspector value/widget type. In those cases update the
corresponding typed enum/struct or executor contract first; do not silently
invent another JSON convention.

### Filter validation checklist

- The definition is included by `Filters/Inspector.json`.
- `ID` is unique and stable.
- Every `Runtime.Shaders.Phases` path resolves to a `.glsl` file.
- Every pass phase exists in `Shaders.Phases`.
- Every pass resource exists in `Current`, `Next`, `Output`, or `Resources.Temps`.
- Every `Parameter` path matches an inspector section and parameter.
- The operation writes all in-bounds output pixels.
- The merge phase writes the declared `Output` resource.
- The filter behaves correctly at zero strength, full strength, and with a mask.
- Shader validation uses TerraForge's include preprocessing and field-format
  define; running a raw GLSL validator directly on custom `#include` files is
  not equivalent to the runtime shader path.
