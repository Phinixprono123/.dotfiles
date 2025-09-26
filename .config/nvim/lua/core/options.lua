-- set options
vim.opt.tabstop = 2
vim.opt.softtabstop = 2
vim.opt.shiftwidth = 2
vim.opt.expandtab = true
vim.opt.autoindent = true
vim.opt.smartindent = true
vim.opt.clipboard = "unnamedplus"
vim.opt.termguicolors = true
vim.opt.undofile = true
vim.opt.undodir = os.getenv("~/.cache/nvim/undo")
vim.opt.mouse = ""

-- global options
vim.g.mapleader = " "
vim.g.maplocalleader = " "

-- enable line numbers
vim.wo.number = true
vim.wo.relativenumber = true

vim.api.nvim_set_hl(0, "LineNr", { fg = "#75715E", bg = "none" })
vim.api.nvim_set_hl(0, "CursorLineNr", { fg = "#75715E", bg = "none" })

-- fix diagnostics
vim.diagnostic.config({
	virtual_text = {
		prefix = "●",
		severity = { min = vim.diagnostic.severity.WARN },
		source = true,
	},
	signs = true,
	update_in_insert = false,
	float = {
		border = "rounded",
		source = "always",
	},
})
