# CmdToolSearch Tool Specification

## Directory Layout

* LIBEXEC_DIR (default: /usr/libexec)
  * cmdtoolsearch
    * _tool_name_
      * run (executable)
        * search in the working directory
        * env:
          * SEARCH_PATTERN : the search pattern
          * SEARCH_FILE_CONTENTS : 1 if search in file contents, empty otherwise
          * ON_HDD : 1 if the working directory is in the HDD, empty otherwise
        * return 0 if successful
      * check (executable)
        * return 0 if the tool is installed, 1 otherwise
      * metadata.json
        * keys
          * separator: "nul" | "newline"
