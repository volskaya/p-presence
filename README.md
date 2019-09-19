[//]: #tech "C++, libgit2, nlohmann/json, Boost, Boost Asio, Boost Beast"

This is a server for use with separately developed, client side extensions,
within code editors.

Client extensions send out their active buffers file paths to the Presence
server, which then, using _libgit2_, searches for the git repository associated
with the path.

Presence then tracks the commits inside git and updates Discord's Rich Presence
with something like "Working on Presence, Ahead by 3 commits"

- Commits are remembered, during the session, to enable support for "Pushed 3 of
  6" status
- Presence server implants hooks, upon discovering _.git_. Hooks cleaned up
  after the Server stops monitoring the repository.

Supports Windows, MacOS, Linux

## Args

| Arg                 | Description                                                                                              |
| ------------------- | -------------------------------------------------------------------------------------------------------- |
| -i, --id            | Custom Discord app ID                                                                                    |
| -v, --verbose       | Print info to STDOUT                                                                                     |
| -r, --replace       | Send shutdown notice to an existing server                                                               |
| -p, --port          | Port for the rest server                                                                                 |
| --language          | Overwrite displayed language                                                                             |
| -I, --default-icons | Fallback to default lang icons. If this is used, custom --id is ignored                                  |
| -U, --auto-update   | Updates session every 15 sec, without waiting for events, dispatched from .git hooks                     |
| --dont-await-repo   | By default, the server will wait for first client to send a valid path, before updating Discord Presence |
| --prevent-shutdown  | Prevent the server from shutting down, when no more client sessions remaining                            |
| -h, --help          |                                                                                                          |
| --version           |                                                                                                          |

## Every request Expects:

- editor: string
- id: int
- params: {}, where params are:

| Route           | Params       |
| --------------- | ------------ |
| set_path        | path: string |
| get_path        |              |
| is_running      |              |
| ping            |              |
| git_change      | name: string |
| im_leaving      |              |
| handle_get_info |              |
| shutdown        |              |

## Built with

- Boost - https://www.boost.org/
- libgit2 - https://libgit2.org/
- https://github.com/nlohmann/json
