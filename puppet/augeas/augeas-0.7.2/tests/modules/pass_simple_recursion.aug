module Pass_simple_recursion =

let rec lns =
  let lbr = del "<" "<" in
  let rbr = del ">" ">" in
  let k   = [ key /[a-z]+/ ] in
  let node = [ label "S" . lbr . lns . rbr ] in
  let b = node | k in
  b*

(* let rec lns = [ key "a" . lns ] | [ key "a" ] *)
test lns get "<x>" = { "S" { "x" } }

test lns put "<x>" after rm "nothing" = "<x>"

test lns put "<<x>>" after rm "nothing" = "<<x>>"

test lns put "<x><x>" after rm "/S[2]" = "<x>"

test lns put "<x>" after clear "/S/S/S/x" = "<x<<x>>>"


(* Start with { "S" { "x" } } and modify to { "S" { "S" { "x" } } } *)
test lns put "<x>" after
  insa "S" "/S";
  clear "/S[2]/S/x";
  rm "/S[1]" = "<<x>>"

test lns get "<<<x><x>><x>><x>" =
  { "S"
    { "S"
      { "S" { "x" } }
      { "S" { "x" } } }
    { "S" { "x" } } }
  { "S" { "x" } }

test lns put "<<<x><x>><x>><x>" after rm "/S[1]/S[1]/S[1]" =
  "<<<x>><x>><x>"


test lns get "<<yo>><zulu>" =
  { "S" { "S" { "yo" } } }
  { "S" { "zulu" } }

(* Some pathological tests for nullable lenses *)

let rec prim_nullable = [ prim_nullable . key /x/ ] | del /s*/ ""
test prim_nullable get "sx" = { "x" }
test prim_nullable get "x" = { "x" }

let rec ambig = [ ambig . label "x" ] | del /s+/ "s"
test ambig get "" = *
test ambig get "s" = *

(* Test link filtering. These tests cause seemingly ambiguous parses, which
 * need to be disambiguated by filtering links in the Earley graph. See
 * section 5.3 in the paper *)
let rec unamb1 = [ label "x" . unamb1 . store /y/ ] | [ key "z" ]
test unamb1 get "zyy" = { "x" = "y" { "x" = "y" { "z" } } }


let rec unamb2 = del /u*/ "" . [ unamb2 . key /x/ ] | del /s*/ ""
test unamb2 get "sx" = { "x" }
test unamb2 get "x" = { "x" }

(* Test proper handling of '?'; bug #119 *)
let rec maybe = [ del "a" "a" . maybe . del "b" "b" ]?
test maybe get "aabb" = { { } }
