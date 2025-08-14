vim.opt.tabstop = 2
vim.opt.softtabstop = 2
vim.opt.shiftwidth = 2
vim.opt.expandtab = true
vim.opt.autoindent = true
vim.opt.smartindent = true
vim.opt.clipboard = "unnamedplus"
vim.opt.termguicolors = true

vim.g.mapleader = " "

vim.wo.number = true
vim.wo.relativenumber = true

vim.api.nvim_set_hl(0, "LineNr", { fg = "#75715E", bg = "none" })
vim.api.nvim_set_hl(0, "CursorLineNr", { fg = "#75715E", bg = "none" })

vim.diagnostic.config({
	virtual_text = {
		prefix = "●", -- Customize prefix if desired
		severity = { min = vim.diagnostic.severity.WARN }, -- Only show WARN and above
		source = true, -- Show the linter source
	},
	signs = true, -- Enable signs in the sign column
	update_in_insert = false, -- Don't update diagnostics in insert mode
	float = {
		border = "rounded", -- Style for the float window
		source = "always", -- Show source in float
	},
})

vim.opt.mouse = ""
