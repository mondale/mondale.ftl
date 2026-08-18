" Turn on vim-only features (not vi)
:set nocompatible

" Background color
:colo darkblue

" Pretty sure this bit sets tabs to be double spaces.
:set expandtab
:set tabstop=2
:set softtabstop=2
:set shiftwidth=2

" Go, go gadget syntax highlighting!
:syn on

" Ignore case during search, highlight during search.
:set ignorecase
:set incsearch
:set hlsearch

" Stop all the swapfile litter
set noswapfile

" Always display line numbers.
:set number

" Write on buffer change events, whatever that means.
:set autowrite

" Don't wrap lines.
:set nowrap

" Keep 50 lines of history
set history=50

" Always show cursor position
set ruler

" Always show mode
set showmode

" My tired eyes need a different font.
set gfn=Consolas\ 14

" Run clang-format on every save
function! FormatOnSave()
  " Save current cursor and view state
  let l:view = winsaveview()
  
  " Run clang-format on the whole buffer silently
  silent! %!clang-format
  
  " Restore cursor and view state
  call winrestview(l:view)
endfunction

" Trigger automatically before saving C and C++ files
autocmd BufWritePre *.c,*.cc,*.cpp,*.h,*.hpp call FormatOnSave()
autocmd BufRead,BufNewFile *.c,*.cc,*.cpp,*.h,*.hpp set filetype=cpp

" Markdown behaviors: wrap on word boundaries at 80 columns.
autocmd FileType markdown setlocal wrap linebreak colorcolumn=80 textwidth=80
