# CmdToolSearch

This is intended to replace `kio-filenamesearch` as the search provider, when Baloo is not enabled.

It has an extensible architecture. The user can specify which "tool" to use in each invocation. New tools
can easily be added as shell scripts.

## Usage

Prefix search string with "@tool_name" to specify tool to use.

Examples:
- `@rg search text`: search file content with `ripgrep`
- `@fd filename`: search file name with `fd`
- `search term`: search with the default tool, `fd` for file name search, `rg` for content. 

It's not limited to search. Any program that outputs a list of paths can be made a tool:
- `@git-status`: list all modified and untracked files in the current repo.
- `@locate filename`: search files in `locate` db.

When the current directory is not local, or the default search tools are not found, it will fall back to `kio-filenamesearch`. The user can also use `@plain` to explicitly fall back to it.

## How it finds tools

Tools are subdirectories under "$HOME/.local/lib/libexec/cmdtoolsearch" and "/usr/lib/libexec/cmdtoolsearch". If they contain duplicate entries, the former takes precedence.

Two text files, "default_file_name_search" and "default_file_content_search", in the above dirs, specify the default tools. Each contains a list of tool names, one tool name per line. When the user doesn't specify a tool using "@tool", the first available tool in the list will be used. If none in the list is available, `kio-filenamesearch` will be used as the final fallback.

## How to make a new tool

A tool dir contains 3 files:

* run: an executable.
  * It will be executed in the search directory (the current directory in Dolphin).
  * Arguments are passed in env:
    * SEARCH_PATTERN : the search pattern
    * SEARCH_FILE_CONTENTS : 1 if search in file contents, empty otherwise
    * ON_HDD : 1 if the working directory is in the HDD, empty otherwise
  * It should output paths, separated by NUL or `\n`, indicated in `metadata.json` below
  * It should return 0 if successful, non-zero otherwise
* check: an executable
  * It should return 0 if the tool is available (e.g. the `rg` command is installed), non-zero otherwise
* metadata.json
  * keys
    * separator: "nul" | "newline"