" Vim syntax file
" Language: Capsule
" Filetype: capsule
" Path: ~/.vim/syntax/capsule.vim

if exists("b:current_syntax")
  finish
endif

" Keywords
syn keyword capsuleKeyword namespace capsule

" Types
syn keyword capsuleType u32 u64 i32 i64 i16 u16 i8 u8 bool string vector

" Attributes (Tokens starting with @)
syn match capsuleAttribute "@\w\+"

" Highlighting Links
hi def link capsuleKeyword Statement
hi def link capsuleType    Type
hi def link capsuleAttribute PreProc

let b:current_syntax = "capsule"
