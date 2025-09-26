return {
	"stevearc/conform.nvim",
	lazy = true,
	opts = {
		formatters_by_ft = {
			lua = { "stylua" },
			python = { "isort", "ruff" },
			javascript = { "prettierd" },
			typescript = { "prettierd" },
			c = { "clang-format" },
			cpp = { "clang-format" },
			html = { "prettierd" },
			css = { "prettierd" },
			json = { "prettierd" },
			md = { "prettierd" },
			rust = { "rustfmt" },
		},
		format_on_save = {
			timeout_ms = 5000,
			lsp_format = "fallback",
		},
	},
}
