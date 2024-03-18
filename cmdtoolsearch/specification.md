# CmdToolSearch Tool Specification

## Directory Layout

* LIBEXEC_DIR (default: /usr/libexec)
  * cmdtoolsearch
    * _tool_name_
      * run (executable)
        * search in the working directory
        * env:
          * SEARCH_TERM : search term
          * CHECK_CONTENT : search in file contents
        * return 0 if successful
      * check (executable)
        * return 0 if the tool is installed, 1 otherwise
      * metadata.json
        * keys
          * separator: "nul" | "newline"
