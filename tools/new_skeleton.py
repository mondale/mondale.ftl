#!/usr/bin/env python3

import sys
import pathlib

def main():
    if len(sys.argv) < 2:
        print("Error: Missing argument. Usage: ./tools/new_skeleton.py <directory/basename>", file=sys.stderr)
        sys.exit(1)

    target = pathlib.Path(sys.argv[1])
    directory = target.parent
    basename = target.name

    # Check if BUILD file exists; if not, print an error and stop.
    build_path = (directory / "BUILD") if directory != pathlib.Path('.') else pathlib.Path("BUILD")
    if not build_path.exists():
        print(f"Error: BUILD file not found in directory '{directory if directory != pathlib.Path('.') else '.'}'.", file=sys.stderr)
        return

    # Define paths for the new files
    h_path = directory / f"{basename}.h"
    cc_path = directory / f"{basename}.cc"
    test_path = directory / f"{basename}_test.cc"

    # If any of the target files already exist, print a warning and do nothing.
    existing_files = [p for p in (h_path, cc_path, test_path) if p.exists()]
    if existing_files:
        print(f"Error: One or more target files already exist: {', '.join(str(p) for p in existing_files)}", file=sys.stderr)
        return

    # Derive naming conventions
    namespace = directory.name if directory != pathlib.Path('.') else ""
    class_name = "".join(word.capitalize() for word in basename.split("_"))
    
    if namespace:
        include_guard = f"{namespace.upper()}_{basename.upper()}_H_"
        header_include = f"{namespace}/{basename}.h"
    else:
        include_guard = f"{basename.upper()}_H_"
        header_include = f"{basename}.h"

    # File contents
    h_content = f"""#ifndef {include_guard}
#define {include_guard}

namespace {namespace} {{

class {class_name} final {{
 public:
 private:
}};

}}  // namespace {namespace}

#endif  // #ifndef {include_guard}
"""

    cc_content = f"""#include "{header_include}"

namespace {namespace} {{

//

}}  // namespace {namespace}
"""

    test_content = f"""#include "{header_include}"
#include "testing/testing.h"

namespace {{

TEST(EmptyTest) {{
  // TODO
}}

}}  // namespace
"""

    build_snippet = f"""
cc_library(
  name = "{basename}",
  hdrs = ["{basename}.h"],
  srcs = ["{basename}.cc"],
  deps = [],
)

cc_test(
  name = "{basename}_test",
  srcs = ["{basename}_test.cc"],
  deps = [":{basename}",
          "//testing",
  ],
)
"""

    # Write out the files
    try:
        h_path.write_text(h_content, encoding="utf-8")
        cc_path.write_text(cc_content, encoding="utf-8")
        test_path.write_text(test_content, encoding="utf-8")

        # Append to BUILD file
        with open(build_path, "a", encoding="utf-8") as f:
            f.write(build_snippet)
            
        print(f"Successfully created skeleton files for {basename} in {directory if directory != pathlib.Path('.') else '.'}")
    except Exception as e:
        print(f"Error: Failed to write files or update BUILD: {e}", file=sys.stderr)
        sys.exit(1)

if __name__ == "__main__":
    main()
