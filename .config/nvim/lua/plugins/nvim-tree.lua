return {
  "nvim-tree/nvim-tree.lua",
  lazy = false,
  config = function()
    require("nvim-tree").setup {}
	vim.keymap.set("n", "<leader>e", function()
	  vim.cmd("silent! NvimTreeToggle")
	end, { desc = "Toggle NvimTree silently" })
	vim.keymap.set("n", "<leader>h", function()
	  vim.cmd("NvimTreeFocus")
	end, { desc = "Focus NvimTree window" })
  end,
}

