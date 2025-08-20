return {
	{
		"mason-org/mason.nvim",
		opts = {},
		config = function()
			require("mason").setup({
				ensure_installed = {
					"prettier",
					"stylua",
					"isort",
				},

				automatic_install = true,
			})
		end,
	},
	{
		"mason-org/mason-lspconfig.nvim",
		opts = {},
		dependencies = {
			{ "mason-org/mason.nvim", opts = {} },
			"neovim/nvim-lspconfig",
		},
		config = function()
			require("mason-lspconfig").setup({
				ensure_installed = {
					"clangd",
					"lua_ls",
					"pyright",
					"ts_ls",
					"bashls",
					"asm_lsp",
					"ruby_lsp",
					"cssls",
					"html",
				},
			})
		end,
	},
	{
		"WhoIsSethDaniel/mason-tool-installer.nvim",
		config = function()
			require("mason-tool-installer").setup({
				run_on_start = true,
				ensure_installed = {
					"stylua",
					"ruff",
					"clang-format",
					"isort",
					"black",
					"vale",
					"eslint_d",
					"luacheck",
					"ast_grep",
				},
			})
		end,
	},
}
