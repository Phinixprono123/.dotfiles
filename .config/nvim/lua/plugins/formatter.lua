return {
  {
    "stevearc/conform.nvim",
    opts = function(_, opts)
      -- Add or override formatters safely
      opts.formatters_by_ft = vim.tbl_deep_extend("force", opts.formatters_by_ft or {}, {
        lua = { "stylua" },
        javascript = { "prettierd" },
        typescript = { "prettierd" },
        yaml = { "prettierd" },
        css = { "prettierd" },
        html = { "prettierd" },
        less = { "prettierd" },
        json = { "prettierd" },
        python = { "isort", "ruff" },
        c = { "clang-format" },
        cpp = { "clang-format" },
        -- Add more filetypes here
      })

      return opts
    end,
  },
}
