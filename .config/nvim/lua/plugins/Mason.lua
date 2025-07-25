return {
  {
    "williamboman/mason.nvim",
    opts = {
      ensure_installed = {
        "ruff",
        "pyright",
        "stylua",
        "shellcheck",
        "shfmt",
        "flake8",
      },
    },
  },
}
