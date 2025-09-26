return {
	"mfussenegger/nvim-lint",
  lazy = true,
	event = { "BufReadPost", "BufNewFile" },
	config = function()
		require("lint").linters_by_ft = {
			lua = { "luacheck" },
			markdown = { "vale" },
			python = { "ruff" },
			javascript = { "eslint_d" },
			typescript = { "eslint_d" },
		}
	end,
}
