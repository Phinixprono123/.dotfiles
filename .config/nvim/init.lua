-- OPTIONS
vim.opt.tabstop = 2
vim.opt.softtabstop = 2
vim.opt.shiftwidth = 2
vim.opt.expandtab = true
vim.opt.autoindent = true
vim.opt.smartindent = true
vim.opt.ignorecase = true
vim.opt.smartcase = true
vim.opt.scrolloff = 8
vim.opt.clipboard = "unnamedplus"
vim.opt.termguicolors = true
vim.opt.undofile = true
vim.opt.undodir = os.getenv("~/.cache/nvim/undo")
vim.opt.mouse = ""
vim.opt.updatetime = 100
vim.opt.timeoutlen = 300
vim.opt.ttimeoutlen = 0
vim.opt.lazyredraw = true
vim.opt.synmaxcol = 200
vim.opt.redrawtime = 1500
vim.opt.swapfile = false
vim.opt.backup = false
vim.opt.fillchars = { eob = " " }

vim.g.loaded_ruby_provider = 0
vim.g.loaded_perl_provider = 0
vim.g.loaded_python_provider = 0
vim.g.loaded_python3_provider = 0
vim.g.loaded_node_provider = 0

-- global options
vim.g.mapleader = " "
vim.g.maplocalleader = " "

-- enable line numbers
vim.wo.number = true
vim.wo.relativenumber = true

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

-- AUTOCMDS
vim.api.nvim_create_autocmd("BufReadPost", {
	callback = function()
		local mark = vim.api.nvim_buf_get_mark(0, '"')
		local lcount = vim.api.nvim_buf_line_count(0)
		if mark[1] > 0 and mark[1] <= lcount then
			pcall(vim.api.nvim_win_set_cursor, 0, mark)
		end
	end,
})

-- KEYMAPS
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

-- LAZY
-- Bootstrap lazy.nvim
local lazypath = vim.fn.stdpath("data") .. "/lazy/lazy.nvim"
if not (vim.uv or vim.loop).fs_stat(lazypath) then
	local lazyrepo = "https://github.com/folke/lazy.nvim.git"
	local out = vim.fn.system({ "git", "clone", "--filter=blob:none", "--branch=stable", lazyrepo, lazypath })
	if vim.v.shell_error ~= 0 then
		vim.api.nvim_echo({
			{ "Failed to clone lazy.nvim:\n", "ErrorMsg" },
			{ out, "WarningMsg" },
			{ "\nPress any key to exit..." },
		}, true, {})
		vim.fn.getchar()
		os.exit(1)
	end
end
vim.opt.rtp:prepend(lazypath)

vim.g.mapleader = " "
vim.g.maplocalleader = "\\"

require("lazy").setup({
	-- PLUGINS
	{
		"catgoose/nvim-colorizer.lua",
		lazy = true,
		event = "BufReadPre",
		opts = {},
		config = function()
			require("colorizer").setup({
				filetypes = { "*" },
				buftypes = {},
				user_commands = true,
				lazy_load = false,
				user_default_options = {
					names = false,
					names_opts = {
						lowercase = true,
						camelcase = true,
						uppercase = true,
						strip_digits = false,
					},
					-- Example: { cool = "#107dac", ["notcool"] = "ee9240" }
					names_custom = false,
					RGB = true,
					RGBA = true,
					RRGGBB = true,
					RRGGBBAA = true,
					AARRGGBB = true,
					rgb_fn = true,
					hsl_fn = true,
					css = true,
					css_fn = true,
					tailwind = true,
					tailwind_opts = { update_names = false },
					sass = { enable = true, parsers = { "css" } },
					xterm = true,
					mode = "background",
					virtualtext = "■",
					virtualtext_inline = false,
					virtualtext_mode = "foreground",
					always_update = true,
					hooks = {
						disable_line_highlight = false,
					},
				},
			})
		end,
	},
	-- Colorscheme
	{
		"AlexvZyl/nordic.nvim",
		lazy = false,
		priority = 1000,
		config = function()
			require("nordic").setup({
				nvim_tree = {
					style = "classic",
				},
			})
			require("nordic").load()
		end,
	},
	-- TODO Comments
	{
		"folke/todo-comments.nvim",
		enable = false,
		lazy = true,
		dependencies = { "nvim-lua/plenary.nvim" },
		opts = {},
	},
	-- Formatter
	{
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
	},
	-- Copilot
	{
		"github/copilot.vim",
		config = function()
			vim.g.copilot_no_tab_map = true
			vim.g.copilot_assume_mapped = true
			vim.g.copilot_tab_fallback = ""

			vim.g.copilot_filetypes = {
				["*"] = false,
				["lua"] = true,
				["python"] = true,
				["sh"] = true,
				["zsh"] = true,
				["bash"] = true,
				["yaml"] = true,
				["json"] = true,
				["javascript"] = true,
				["typescript"] = true,
				["rust"] = true,
				["go"] = true,
				["c"] = true,
				["cpp"] = true,
				["java"] = true,
			}
			-- Accept Copilot
			vim.keymap.set("i", "<S-CR>", 'copilot#Accept("\\<CR>")', {
				expr = true,
				replace_keycodes = false,
				desc = "Accept Copilot",
			})
		end,
	},
	-- LSP
	{
		"neovim/nvim-lspconfig",
		enabled = true,
		event = { "BufReadPre", "BufNewFile" },
		config = function()
			-- List your servers here
			local servers =
				{ "lua_ls", "pyright", "clangd", "ts_ls", "ruby_ls", "bashls", "asm_lsp", "cssls", "html-lsp" }

			-- Global defaults for all servers
			vim.lsp.config("*", {
				on_attach = function(client, bufnr)
					local map = function(mode, lhs, rhs)
						vim.keymap.set(mode, lhs, rhs, { buffer = bufnr, silent = true })
					end

					-- Common LSP keymaps
					map("n", "K", vim.lsp.buf.hover)
					map("n", "<leader>rn", vim.lsp.buf.rename)
					map("n", "gd", vim.lsp.buf.definition)
					map("n", "gD", vim.lsp.buf.declaration)
					map("n", "gr", vim.lsp.buf.references)
					map("n", "<leader>ca", vim.lsp.buf.code_action)

					vim.bo[bufnr].omnifunc = "v:lua.vim.lsp.omnifunc"
				end,
			})

			-- Load and enable each server
			for _, server in ipairs(servers) do
				local ok, conf = pcall(require, "lsp." .. server)
				if ok then
					vim.lsp.config(server, conf)
				end
				vim.lsp.enable(server)
			end
		end,
	},
	-- Mason installer
	{
		"mason-org/mason.nvim",
		enabled = true,
		opts = {},
		config = function()
			require("mason").setup()
		end,
	},
	{
		"mason-org/mason-lspconfig.nvim",
		enabled = true,
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
		enabled = true,
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
					"prettierd",
				},
			})
		end,
	},
	-- Lualine
	{
		"nvim-lualine/lualine.nvim",
		priority = 1000,
		lazy = false,
		dependencies = { "nvim-tree/nvim-web-devicons" },
		config = function()
			require("lualine").setup({
				options = {
					icons_enabled = true,
					theme = "nord",
					component_separators = { left = "", right = "" },
					section_separators = { left = "", right = "" },
					disabled_filetypes = {
						statusline = {},
						winbar = {},
					},
					ignore_focus = {},
					always_divide_middle = true,
					always_show_tabline = true,
					globalstatus = false,
					refresh = {
						statusline = 1000,
						tabline = 1000,
						winbar = 1000,
						refresh_time = 16, -- ~60fps
						events = {
							"WinEnter",
							"BufEnter",
							"BufWritePost",
							"SessionLoadPost",
							"FileChangedShellPost",
							"VimResized",
							"Filetype",
							"CursorMoved",
							"CursorMovedI",
							"ModeChanged",
						},
					},
				},
				sections = {
					lualine_a = { "mode" },
					lualine_b = { "branch", "diff", "diagnostics" },
					lualine_c = { "filename" },
					lualine_x = { "encoding", "fileformat", "filetype" },
					lualine_y = { "progress" },
					lualine_z = { "location" },
				},
				inactive_sections = {
					lualine_a = {},
					lualine_b = {},
					lualine_c = { "filename" },
					lualine_x = { "location" },
					lualine_y = {},
					lualine_z = {},
				},
				tabline = {},
				winbar = {},
				inactive_winbar = {},
				extensions = {},
			})
		end,
	},

	-- Markdown
	{
		"MeanderingProgrammer/render-markdown.nvim",
		dependencies = { "nvim-treesitter/nvim-treesitter", "nvim-tree/nvim-web-devicons" },
		ft = { "markdown" },
		opts = {},
	},

	-- Linting
	{
		"mfussenegger/nvim-lint",
		lazy = true,
		-- event = { "BufReadPost", "BufNewFile" },
		event = "BufWritePost",
		config = function()
			require("lint").linters_by_ft = {
				lua = { "luacheck" },
				markdown = { "vale" },
				python = { "ruff" },
				javascript = { "eslint_d" },
				typescript = { "eslint_d" },
			}
		end,
	},

	-- nvim-autopairs
	{
		"windwp/nvim-autopairs",
		lazy = true,
		event = "InsertEnter",
		config = true,
	},

	-- nvim-cmp
	{
		"hrsh7th/nvim-cmp",
		event = "InsertEnter",
		dependencies = {
			"hrsh7th/cmp-nvim-lsp",
			"hrsh7th/cmp-buffer",
			"hrsh7th/cmp-path",
			"hrsh7th/cmp-cmdline",
			"hrsh7th/cmp-calc",
			"hrsh7th/cmp-cmdline",
			"L3MON4D3/LuaSnip",
			"rafamadriz/friendly-snippets",
			"onsails/lspkind.nvim",
		},
		config = function()
			local cmp = require("cmp")
			local luasnip = require("luasnip")
			local lspkind = require("lspkind")

			require("luasnip.loaders.from_vscode").lazy_load()

			cmp.setup({
				completion = {
					completeopt = "menu,menuone,noselect",
				},
				snippet = {
					expand = function(args)
						luasnip.lsp_expand(args.body)
					end,
				},
				window = {
					completion = cmp.config.window.bordered({
						side_padding = 0,
						border = { "╭", "─", "╮", "│", "╯", "─", "╰", "│" },
						-- winhighlight = "Normal:Pmenu,FloatBorder:PmenuBorder,CursorLine:PmenuSel",
					}),
					documentation = cmp.config.window.bordered({ -- right-side docs
						side_padding = 0,
						border = "rounded", -- "single", "double", "rounded", or custom table
						-- winhighlight = "NormalFloat:NormalFloat,FloatBorder:FloatBorder",
					}),
				},
				vim.api.nvim_set_hl(0, "PmenuBorder", { fg = "#8BE9FD", bg = "NONE" }),
				vim.api.nvim_set_hl(0, "Pmenu", { fg = "#ECEFF4", bg = "#2E3440" }),
				vim.api.nvim_set_hl(0, "PmenuSel", { fg = "#2E3440", bg = "#88C0D0" }),
				vim.api.nvim_set_hl(0, "FloatBorder", { fg = "#8BE9FD", bg = "NONE" }),

				mapping = cmp.mapping.preset.insert({
					["<C-b>"] = cmp.mapping.scroll_docs(-4),
					["<C-f>"] = cmp.mapping.scroll_docs(4),
					["<C-Space>"] = cmp.mapping.complete(),
					["<C-e>"] = cmp.mapping.abort(),
					["<CR>"] = cmp.mapping.confirm({ select = true }),
					["<Tab>"] = cmp.mapping(function(fallback)
						if cmp.visible() then
							cmp.select_next_item()
						elseif luasnip.expand_or_jumpable() then
							luasnip.expand_or_jump()
						else
							fallback()
						end
					end, { "i", "s" }),
					["<S-Tab>"] = cmp.mapping(function(fallback)
						if cmp.visible() then
							cmp.select_prev_item()
						elseif luasnip.jumpable(-1) then
							luasnip.jump(-1)
						else
							fallback()
						end
					end, { "i", "s" }),
				}),
				sources = cmp.config.sources({
					{ name = "nvim_lsp" },
					{ name = "calc" },
					{ name = "cmdline" },
					{ name = "luasnip" },
					{ name = "buffer" },
					{ name = "path" },
				}),
				formatting = {
					format = lspkind.cmp_format({
						maxwidth = 50,
						ellipsis_char = "...",
						mode = "symbol_text",
						format_symbol = {
							path = "󰉋",
							buffer = "󰈚",
						},
					}),
				},
			})

			-- Cmdline completion (optional)
			cmp.setup.cmdline({ "/", "?" }, {
				mapping = cmp.mapping.preset.cmdline(),
				sources = {
					{ name = "buffer" },
				},
			})
			cmp.setup.cmdline(":", {
				mapping = cmp.mapping.preset.cmdline(),
				sources = cmp.config.sources({
					{ name = "path" },
				}, {
					{ name = "cmdline" },
				}),
			})
		end,
	},

	-- nvim-notify
	{
		"rcarriga/nvim-notify",
	},

	-- nvim-tree
	{
		"nvim-tree/nvim-tree.lua",
		lazy = false,
		config = function()
			require("nvim-tree").setup({})
		end,
	},

	-- Telescope
	{
		"nvim-telescope/telescope.nvim",
		tag = "0.1.8",
		dependencies = { "nvim-lua/plenary.nvim" },
		event = "VeryLazy",
		config = function()
			local builtin = require("telescope.builtin")
			vim.keymap.set("n", "<leader>ff", builtin.find_files, { desc = "Telescope find files" })
			vim.keymap.set("n", "<leader>fg", builtin.live_grep, { desc = "Telescope live grep" })
		end,
	},
	{
		"nvim-telescope/telescope-ui-select.nvim",
		event = "VeryLazy",
		config = function()
			require("telescope").setup({
				extensions = {
					["ui-select"] = {
						require("telescope.themes").get_dropdown({}),
					},
				},
			})
			require("telescope").load_extension("ui-select")
		end,
	},

	-- Treesitter
	{
		"nvim-treesitter/nvim-treesitter",
		enabled = false,
		branch = "master",
		lazy = false,
		build = ":TSUpdate",
		config = function()
			require("nvim-treesitter.configs").setup({
				ensure_installed = {
					"c",
					"lua",
					"vim",
					"vimdoc",
					"query",
					"markdown",
					"markdown_inline",
					"cpp",
					"json",
					"jsonc",
					"python",
					"rust",
					"ruby",
					"yuck",
					"css",
					"scss",
					"javascript",
					"typescript",
					"zig",
					"asm",
					"yaml",
					"toml",
					"bash",
					"bass",
					"c_sharp",
					"cooklang",
					"csv",
					"d",
					"dart",
					"desktop",
					"diff",
					"erlang",
					"fish",
					"gdscript",
					"git_config",
					"gitcommit",
					"go",
					"hyprlang",
					"http",
					"java",
					"kotlin",
					"make",
					"nix",
					"php",
					"powershell",
					"properties",
					"sql",
					"tmux",
				},
				highlight = {
					enable = true,
					max_file_lines = 100000,
				},
				indent = { enable = true },
			})
		end,
	},
	checker = { enabled = true },

	performance = {
		cache = { enabled = true },
		reset_packpath = true,
		rtp = {
			disabled_plugins = {
				"matchit",
				"tohtml",
				"tutor",
				"tarPlugin",
				"zipPlugin",
				"gzip",
				"matchparen",
				"netrwPlugin",
			},
		},
	},
})
