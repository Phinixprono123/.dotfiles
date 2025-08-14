return {
	"catgoose/nvim-colorizer.lua",
	event = "BufReadPre",
	opts = {},
	config = function()
		require("colorizer").setup({
			filetypes = { "*" },
			buftypes = {},
			user_commands = true,
			lazy_load = false,
			user_default_options = {
				names = false,
				names_opts = {
					lowercase = true,
					camelcase = true,
					uppercase = true,
					strip_digits = false,
				},
				-- Example: { cool = "#107dac", ["notcool"] = "ee9240" }
				names_custom = false,
				RGB = true,
				RGBA = true,
				RRGGBB = true,
				RRGGBBAA = true,
				AARRGGBB = true,
				rgb_fn = true,
				hsl_fn = true,
				css = true,
				css_fn = true,
				tailwind = true,
				tailwind_opts = { update_names = false },
				sass = { enable = true, parsers = { "css" } },
				xterm = true,
				mode = "background",
				virtualtext = "■",
				virtualtext_inline = false,
				virtualtext_mode = "foreground",
				always_update = true,
				hooks = {
					disable_line_highlight = false,
				},
			},
		})
	end,
}
