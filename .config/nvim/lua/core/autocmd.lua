vim.api.nvim_create_autocmd({ "BufReadPost", "BufNewFile", "InsertLeave" }, {
	group = lint_group,
	callback = function()
		require("lint").try_lint()
	end,
})

-- Return to last edit position when reopening a file
vim.api.nvim_create_autocmd("BufReadPost", {
	callback = function()
		local mark = vim.api.nvim_buf_get_mark(0, '"')
		local lcount = vim.api.nvim_buf_line_count(0)
		if mark[1] > 0 and mark[1] <= lcount then
			pcall(vim.api.nvim_win_set_cursor, 0, mark)
		end
	end,
})
