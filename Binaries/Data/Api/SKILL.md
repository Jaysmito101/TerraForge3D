---
name: api
description: Use the TerraForge3D HTTP API to inspect and control capabilities exposed by the running application.
---

# TerraForge3D API

Use this skill when an application or agent needs to inspect or control the
running TerraForge3D instance over HTTP. Get the public API base URL from the
host application or integration that provided this skill. If this skill was
loaded over HTTP from a URL ending in `/api/skill.md`, remove that suffix to
get the public base URL; retain any path prefix before it. If the skill was
installed as a local file, use the base URL supplied by the host application.
Do not assume a host name or port. Operation routes use the `/api/v1` prefix.
If the host application provides no connection details and this skill was not
loaded over HTTP, request the API base URL from that application instead of
guessing one.

Directory and operation links are route paths. To request one, append the
route path to the public base URL, keeping any path prefix in that base URL.
Concatenate the base URL without its trailing slash and the returned route
path, including the route path's leading slash. Do not resolve a leading-slash
route as a new origin-root URL, since that discards a proxy path prefix.

## Browse one level at a time

Start with the compact operation directory:

```sh
curl "${API_BASE_URL}/api/v1/operations"
```

Set `API_BASE_URL` to the public base URL discovered above, without a trailing
slash.

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
curl "${API_BASE_URL}/api/v1/status"
```

Viewport capture returns JPEG bytes with `Content-Type: image/jpeg`; its
dimensions are returned in `X-Image-Width` and `X-Image-Height`. Other
successful operations return JSON. Errors use `application/problem+json` and
include a stable `code` field.

The playground and skill pages are served by the same API service, so no
external site or package is needed to inspect or try an operation.
