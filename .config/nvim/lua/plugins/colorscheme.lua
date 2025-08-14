return {
    'AlexvZyl/nordic.nvim',
    lazy = false,
    priority = 1000,
    config = function()
	require('nordic').setup({
          nvim_tree = {
            style = "classic",
	  },
	})
        require('nordic').load()
    end
}
