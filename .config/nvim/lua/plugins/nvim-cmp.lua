return {
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
}
