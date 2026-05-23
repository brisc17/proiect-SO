# AI Usage Documentation — Phase 1



## Functions Generated with AI Assistance

### 1. parse_condition()

**Prompt given to AI:**
I described my Report structure with its fields and asked for a function
int parse_condition(const char *input, char *field, char *op, char *value)
that splits a string like "severity:>=:2" or "category:==:road" into
three separate parts: field, operator and value.
Return 1 on success, 0 on failure.

**What was generated:**
A function that uses strchr() to find the ':' separator and split
the three components. It copies the input into a local buffer first
to avoid modifying the original string.

**What I changed:**
Added NULL pointer check at the beginning of the function.
Adjusted buffer sizes to match the constants defined in city_manager.h.

**What I learned:**
strchr() can be used to split a string by setting *ptr = '\0'.
Using a local copy of the input is important so the original
string is not modified.

---

### 2. match_condition()

**Prompt given to AI:**
I described the fields of the Report struct and their types and asked
for a function:
int match_condition(Report *r, const char *field, const char *op, const char *value)
that returns 1 if the record satisfies the condition and 0 otherwise.
String fields support only == and !=.
Integer fields support ==, !=, <, >, <=, >=.

**What was generated:**
A function that branches on the field name using strcmp().
For string fields it uses strcmp() to compare values.
For integer fields it converts the value with atoi() and
performs numeric comparison.

**What I changed:**
Added NULL pointer guard at the top.
Added the id field which was missing from the generated version.
Changed warning messages to match the style used in the rest
of the program.

**What I learned:**
atoi() silently returns 0 for invalid strings, strtol() would
be safer for production code.
The AI correctly handled both string and integer cases on
first generation.



## Critical Evaluation

**Correctness:**
Both functions were logically correct after minor adjustments.

**What I verified line by line:**
- parse_condition: traced manually with input "severity:>=:2"
  to confirm correct splitting
- match_condition: tested each branch with actual report data

**Limitations noticed:**
- atoi() has no error reporting for invalid values
- No support for latitude/longitude filtering
- The id field was missing initially from match_condition

# AI Usage Documentation - Phase 2
**op_remove_district() - fork + execl + symlink fix**
  Prompt given to AI:
    Asked for help with the remove_district operation using fork() and execl() to run
    rm -rf, and fixing a typo in the symlink variable name.
  AI response:
    Fixed typo symplink_path -> symlink_path, added directory existence check before
    fork(), used _exit() in child after execl() failure.
  How I verified it:
    Tested by creating a district, then removing it, confirming directory and symlink
    are both deleted.
  Modifications made:
    Added stat() check to verify district exists before attempting removal.

# AI Usage Documentation Phase 3 — Pipes and Redirects

# Where AI was used

  The architecture for hub_mon_process(), had a template from which I continued on.I also used it as guidence for structuring inter-process communication and for getting ideas on how the processes and pipes should be linked together efficiently or how to fix minor bugs.