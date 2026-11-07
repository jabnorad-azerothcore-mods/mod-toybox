# Contributing

Issues and pull requests are welcome.

This repository is published from a private upstream: each release arrives as a single commit
from a `releases/<version>` pull request. Because of that, pull requests aren't merged here
directly. Instead:

1. Every pull request is built against current AzerothCore master, with codestyle and luacheck
   checks. Please get those passing.
2. An accepted pull request is applied upstream with your commits and authorship kept.
3. It ships in the next release, whose commit credits you with a `Co-authored-by` trailer, and
   that release closes your pull request.

Please keep to AzerothCore's [C++ code standards](https://www.azerothcore.org/wiki/cpp-code-standards)
and the existing style of the addon's Lua.
