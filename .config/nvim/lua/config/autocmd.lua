vim.api.nvim_create_autocmd({ "BufReadPost", "BufNewFile", "InsertLeave" }, {
	group = lint_group,
	callback = function()
		require("lint").try_lint()
	end,
})
