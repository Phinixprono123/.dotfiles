vim.keymap.set("n", "<Leader>gf", function()
	require("conform").format()
end, { desc = "Format current file" })

vim.keymap.set("n", "<leader>ll", function()
	require("lint").try_lint()
end, { desc = "Try linting for the current file" })

vim.keymap.set("n", "<leader>n", function()
	require("telescope").extensions.notify.notify()
end, { desc = "Show notification history" })

vim.keymap.set({ "n", "v" }, "d", function()
	return '"_d'
end, { expr = true, noremap = true })
