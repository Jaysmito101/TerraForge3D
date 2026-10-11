---
name: api
description: Use the TerraForge3D local HTTP API to inspect and control capabilities exposed by the running application.
---

# TerraForge3D API

Use this skill when an application or agent needs to inspect or control the
running TerraForge3D instance over HTTP. The server listens on
`127.0.0.1:9823` by default. `TF3D_API_PORT` changes the port. Operation routes
use the `/api/v1` prefix.

## Browse one level at a time

Start with the compact operation directory:

```sh
curl http://127.0.0.1:9823/api/v1/operations
```

Follow a returned child `path` to open that group. Each group response lists
only its immediate child groups and operations. An operation summary links to
its single-operation details, composed schema, and generated skill page. Read
those links only for the operation you need.

The matching hierarchy is also available from `GET /api/v1/skills`. The root
skill route is `GET /api/skill.md`; each operation skill is linked from its
group entry. The API playground at `GET /api/docs` uses this same step-by-step
browsing flow.

`GET /api/v1/openapi.json` returns the complete OpenAPI document for tools that
need a full contract. For ordinary exploration, use the directory and
operation links so you only load the relevant schema and instructions.

## Call an operation

Use the method and path from the operation details. Path parameters go in the
URL. For `GET`, send remaining values as URL query parameters. For `POST` and
`PATCH`, send remaining values as a JSON object with
`Content-Type: application/json`. The linked composed schema describes the
complete validated argument object, including path parameters.

For example, status is a read-only operation:

```sh
curl http://127.0.0.1:9823/api/v1/status
```

Viewport capture returns JPEG bytes with `Content-Type: image/jpeg`; its
dimensions are returned in `X-Image-Width` and `X-Image-Height`. Other
successful operations return JSON. Errors use `application/problem+json` and
include a stable `code` field.

The playground and skill pages are served by the same local process, so no
external site or package is needed to inspect or try an operation.
