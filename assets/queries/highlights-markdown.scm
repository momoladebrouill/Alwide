;From nvim-treesitter/nvim-treesitter
(atx_heading (inline) @function)
(setext_heading (paragraph) @function)

[
  (atx_h1_marker)
  (atx_h2_marker)
  (atx_h3_marker)
  (atx_h4_marker)
  (atx_h5_marker)
  (atx_h6_marker)
  (setext_h1_underline)
  (setext_h2_underline)
  ] @keyword

[
  (link_title)
  (indented_code_block)
  ] @string

[
  (fenced_code_block_delimiter)
  ] @number

(code_fence_content) @none

[
  (link_destination)
  ] @string

[
  (link_label)
  ] @type

[
  (list_marker_plus)
  (list_marker_minus)
  (list_marker_star)
  (list_marker_dot)
  (list_marker_parenthesis)
  (thematic_break)
  ] @keyword

[
  (block_continuation)
  (block_quote_marker)
  ] @keyword

[
  (backslash_escape)
] @number

(language) @type


(fenced_code_block
  (info_string
    (language) @injection.language)
  (code_fence_content) @injection.content)

((html_block) @injection.content (#set! injection.language "html"))

(document . (section . (thematic_break) (_) @injection.content (thematic_break)) (#set! injection.language "yaml"))

([(minus_metadata) (plus_metadata)] @injection.content (#set! injection.language "yml"))

((inline) @injection.content (#set! injection.language "markdown_inline"))

