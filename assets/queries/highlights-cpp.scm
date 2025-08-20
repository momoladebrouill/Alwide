; Functions

(call_expression
	function: (qualified_identifier
							name: (identifier) @function))

(template_function
	name: (identifier) @function)

(template_method
	name: (field_identifier) @function)

(template_function
	name: (identifier) @function)

(function_declarator
	declarator: (qualified_identifier
								name: (identifier) @function))

(function_declarator
	declarator: (field_identifier) @function)

; Types

((namespace_identifier) @type
	(#match? @type "^[A-Z]"))

(auto) @type

; Constants

(this) @variable.builtin
(null "nullptr" @constant)

; Modules
(module_name
	(identifier) @module)

; Keywords

[
	"catch"
	"class"
	"co_await"
	"co_return"
	"co_yield"
	"constexpr"
	"constinit"
	"consteval"
	"delete"
	"explicit"
	"final"
	"friend"
	"mutable"
	"namespace"
	"noexcept"
	"new"
	"override"
	"private"
	"protected"
	"public"
	"template"
	"throw"
	"try"
	"typename"
	"using"
	"concept"
	"requires"
	"virtual"
	"import"
	"export"
	"module"
	] @keyword

; Strings

(raw_string_literal) @string


(raw_string_literal
	delimiter: (raw_string_delimiter) @injection.language
	(raw_string_content) @injection.content)


(struct_specifier name: (type_identifier) @name body:(_)) @type

(declaration type: (union_specifier name: (type_identifier) @name)) @type

(function_declarator declarator: (identifier) @name) @function

(function_declarator declarator: (field_identifier) @name) @function

(function_declarator declarator: (qualified_identifier scope: (namespace_identifier) @local.scope name: (identifier) @name)) @method

(type_definition declarator: (type_identifier) @name) @type

(enum_specifier name: (type_identifier) @name) @type

(class_specifier name: (type_identifier) @name) @type
