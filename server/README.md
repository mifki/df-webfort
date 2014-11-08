Configuration
=============

The Webfort plugin has no defined configuration file. Instead, it uses
environment variables. The variables it looks for are:

| var              | description                                                                          |
|------------------|--------------------------------------------------------------------------------------|
| `WF_PORT`        | The port number webfort listens on. default: 1234                                    |
| `WF_TURNTIME`    | The amount of time, in seconds, each player has in a turn. default: 600 (10 minutes) |
| `WF_MAX_CLIENTS` | The number of connections that can be opened at any one time. default: 32.           |
