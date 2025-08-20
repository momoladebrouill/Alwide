(tag_name) @function
(erroneous_end_tag_name) @function
(doctype) @constant
(attribute_name) @property
(attribute_value) @string
(comment) @comment

[
  "<"
  ">"
  "</"
  "/>"
] @type

[
  "\""
] @string

((script_element
   (raw_text) @injection.content)
  (#set! injection.language "javascript"))

((style_element
   (raw_text) @injection.content)
  (#set! injection.language "css"))
