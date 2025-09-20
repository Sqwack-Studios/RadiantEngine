vim.opt.makeprg = '.\\CompileGame.bat'

local builtin = require('telescope.builtin')

  vim.keymap.set('n', '<leader>fsdk', function()
    builtin.find_files({
      search_dirs = {
        vim.fn.getcwd(),
        "C:/Program Files/Microsoft Visual Studio/2022/Community/VC/Tools/MSVC/14.44.35207/include",
        "C:/Program Files (x86)/Windows Kits/10/Include/10.0.22621.0"
    }})
  end, { desc = 'Find files incluing SDK/STL' })
