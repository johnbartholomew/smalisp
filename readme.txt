SmaLisp
=======


SmaLisp is a simple lisp dialect, created for fun. It's slow and it has
a very simplistic garbage collector.

The language  has a few  neat features.  Most lisps  provide 'functions'
and  'macros':  functions  receive  arguments  that  have  already  been
evaluated,  and  the result  of the  function is  not evaluated;  macros
receive arguments that  have not been evaluated,  but the  result of the
macro is evaluated.  SmaLisp  adds a lower level form  for which neither
the arguments nor the result  are evaluated,  but the calling context is
passed in as a parameter,  which  allows the closure to perform whatever
evaluations or environment manipulation it needs to.

Effectively,  this lower  level  form,  along  with  some facilities for
direct manipulation  of  explicitly  bound  stack frame objects,  allows
almost all common forms to be implemented in SmaLisp code rather than as
built-in functions.  Of course, SmaLisp code is very slow so in practice
built-ins are preferred.

SmaLisp is  usually interpreted directly  from the  code tree.  However,
I've also started adding a bytecode based evaluator.  Bytecode functions
can be directly created by SmaLisp  code,  which means that it should be
possible to implement the bytecode compiler in SmaLisp.

Unfortunately,   this  distribution   does  not  include   the  bytecode
evaluation code at all (I hope it hasn't been lost...)

SmaLisp is  an interpreter for a  dialect of  lisp.  The dialect  is not
described in a formal specification,  as SmaLisp is intended to serve as
a  base   for  testing  out   language  features,   rather  than   as  a
commercial-grade programming language.


Legal
-----

For licensing terms, see the accompanying license.txt.

# vim: set ts=8 sts=3 sw=3 et ai tw=72 wm=6 wrap:
