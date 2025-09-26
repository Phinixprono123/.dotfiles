-- Try to format current file
vim.keymap.set("n", "<Leader>gf", function()
	require("conform").format()
end, { desc = "Format current file" })

-- Try to lint on the current file
vim.keymap.set("n", "<leader>ll", function()
	require("lint").try_lint()
end, { desc = "Try linting for the current file" })

-- Show notification history
vim.keymap.set("n", "<leader>n", function()
	require("telescope").extensions.notify.notify()
end, { desc = "Show notification history" })

-- Fix "d" for deleting instead of copying
vim.keymap.set({ "n", "v" }, "d", function()
	return '"_d'
end, { expr = true, noremap = true })

-- Toggle nvim tree
vim.keymap.set("n", "<leader>e", function()
	vim.cmd("silent! NvimTreeToggle")
end, { desc = "Toggle NvimTree" })

-- Focus on nvim tree
vim.keymap.set("n", "<leader>h", function()
	vim.cmd("NvimTreeFocus")
end, { desc = "Focus NvimTree window" })
