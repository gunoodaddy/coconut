if &cp | set nocp | endif
let s:cpo_save=&cpo
set cpo&vim
nmap gx <Plug>NetrwBrowseX
nnoremap <silent> <Plug>NetrwBrowseX :call netrw#NetrwBrowseX(expand("<cWORD>"),0)
map <PageDown> 
map <PageUp> 
let &cpo=s:cpo_save
unlet s:cpo_save
set autoindent
set background=dark
set backspace=indent,eol,start
set cscopeprg=/usr/bin/cscope
set cscopetag
set cscopeverbose
set fileencodings=ucs-bom,utf-8,default,latin1
set helplang=en
set history=50
set hlsearch
set incsearch
set nomodeline
set ruler
set shiftwidth=4
set smartindent
set tabstop=4
set tags=./tags
set title
set viminfo='20,\"50
" vim: set ft=vim :
