; General syntax
(command_name) @function

; Environments
(begin
  command: _ @function
  name: (_) @label)

(end
  command: _ @function
  name: (_) @label)

; Sectioning
(chapter command: _ @type)
(part command: _ @type)
(section command: _ @type)
(subsection command: _ @type)
(subsubsection command: _ @type)
(paragraph command: _ @type)
(subparagraph command: _ @type)

; Specific commands
(caption command: _ @function)
(enum_item command: _ @function)

; Brackets, delimiters, operators
"(" @keyword
")" @keyword
"[" @keyword
"]" @keyword
"{" @keyword
"}" @keyword
"$" @keyword
"=" @keyword
"_" @keyword
"^" @keyword

(delimiter) @keyword
(operator) @keyword

; Text within groups
(text) @spell

; Comments
(line_comment) @comment
(block_comment) @comment
(comment_environment) @comment
