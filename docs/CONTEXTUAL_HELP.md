# Contextual Settings Help

ProteusManager keeps setting explanations beside the control they describe.
The small information button can be reached by mouse or keyboard and stays
open after a click so longer text can be read without holding the pointer in
place.

Section and page headings never receive an information button. Their wording
must explain the section directly; contextual help belongs only to an
editable setting or option.

## Writing Rules

Each explanation should remain short and answer these questions:

1. What does the setting change?
2. When is it useful?
3. Which mistake or security risk can it reduce?

Help text must never include live credentials, connection secrets, generated
tokens, or values copied from password fields.

## Current Coverage

The main workflow documents the target language, Ollama model and endpoint,
local or online database mode, local database file, database driver, database
name, host, port, user name, and password. Class to SQL documents its source
folder and audit-field option. Secure Code documents its generation scope,
output directory, and language-specific advanced settings.

## Layout Rules

- Keep the information button directly to the right of its setting.
- Do not place information buttons next to page or section headings.
- Keep controls usable at a 560-pixel-wide application window.
- Use compact spacing and a maximum content width on wide displays.
- Keep section corner radii at 8 pixels.
- Present AI and database state as status, not as another editable setting.

`QmlWorkflowTest` verifies the help controls and checks that action bars remain
inside the main, Class to SQL, and Secure Code pages at the compact reference
size.
