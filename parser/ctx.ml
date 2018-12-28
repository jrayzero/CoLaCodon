(******************************************************************************
 *
 * Seq OCaml 
 * ctx.ml: Context (variable table) definitions 
 *
 * Author: inumanag
 *
 ******************************************************************************)

open Core

(** Variable table description  *)
module Namespace = 
struct
  (** Variable table is a dictionary that maps names to LLVM types *)
  type t = 
    (string, elt list) Hashtbl.t
  (** Describes potential kinf of variable *)
  and elt = 
    el * annotation
  and el = 
    | Var  of Llvm.Types.var_t
    | Type of Llvm.Types.typ_t
    | Func of (Llvm.Types.func_t * string list)
    | Import of t
  (** Each assignable variable is annotated with 
      [base] function pointer, and flags describing does
      it belong to toplevel or not, and is it global or not  *)
  and annotation = 
    { base: Llvm.Types.func_t;
      toplevel: bool;
      global: bool;
      internal: bool }
end

(** Context type *)
type t = 
  { (** context filename *)
    filename: string;
    (** [SeqModule] pointer *)
    mdl: Llvm.Types.func_t;
    (** base function pointer *)
    base: Llvm.Types.func_t;
    (* block pointer *)
    block: Llvm.Types.block_t;
    (** try-catch pointer for expressions *)
    trycatch: Llvm.Types.stmt_t;

    (** stack of blocks. Top of stack is the current (deepest) block.
        Each block maintains a set of variables on stack. *)
    stack: ((string) Hash_set.t) Stack.t;
    (** Variable lookup table *)
    map: Namespace.t;

    (** function that parses a file within current context 
        (used for processing [import] statements) *)
    parse_file: (t -> string -> unit) }

(** [add_block context] pushed a new block to context stack *)
let add_block ctx = 
  Stack.push ctx.stack (String.Hash_set.create ())

(* Returns the list of native POD types *)
let pod_types () = 
  Llvm.Type.(
    [ "void", void; 
      "int", int; 
      "str", str;  
      "seq", seq;
      "bool", bool; 
      "float", float; 
      "byte", byte ]) 

(** [add context name var] adds a variable to the current block *)
let add (ctx: t) ?(toplevel=false) ?(global=false) ?(internal=false) key var =
  let annot = Namespace.
    { base = ctx.base; global; toplevel; internal } in
  let var = (var, annot) in
  begin match Hashtbl.find ctx.map key with
    | None -> 
      Hashtbl.set ctx.map ~key ~data:[var]
    | Some lst -> 
      Hashtbl.set ctx.map ~key ~data:(var :: lst)
  end;
  Hash_set.add (Stack.top_exn ctx.stack) key

(** [init ...] initializes an empty context with toplevel block
    and adds internal POD types to the namespace *)
let init ?(stdlib=true) ?(argv=true) filename mdl base block parse_file =
  let ctx = 
    { filename;
      mdl;
      base;
      block;
      stack = Stack.create ();
      map = String.Table.create ();
      parse_file;
      trycatch = Ctypes.null }
  in
  add_block ctx;

  (* default flags for internal arguments *)
  let toplevel = true in
  let global = true in
  let internal = true in 

  (* initialize POD types *)
  List.iter (pod_types ()) ~f:(fun (key, fn) -> 
    add ctx ~toplevel ~global ~internal
      key @@ Namespace.Type (fn ()));
  
  (* set __argv__ params *)
  if argv then begin
    let args = Llvm.Module.get_args mdl in
    add ctx ~internal ~global ~toplevel 
      "__argv__" (Namespace.Var args)
  end;

  (* load standard library *)
  if stdlib then begin
    let seqpath = Option.value (Sys.getenv "SEQ_PATH") ~default:"" in
    let stdlib_path = sprintf "%s/stdlib.seq" seqpath in
    ctx.parse_file ctx stdlib_path
  end;
  ctx

(** [clear_block context] pops the current block 
    and removes all block variables from vtable *)
let clear_block ctx =
  Hash_set.iter (Stack.pop_exn ctx.stack) ~f:(fun key ->
    match Hashtbl.find ctx.map key with
    | Some [_] -> 
      Hashtbl.remove ctx.map key
    | Some (_ :: items) -> 
      Hashtbl.set ctx.map ~key ~data:items
    | Some [] | None ->
      failwith (sprintf "can't find context variable %s" key))

(** [in_scope context name] checks is a variable [name] present 
    in the current scope and returns it if so *)
let in_scope ctx key =
  match Hashtbl.find ctx.map key with
  | Some (hd :: _) -> Some hd
  | _ -> None

(** [in_block context name] checks is a variable [name] present 
    in the current block and returns it if so *)
let in_block ctx key =
  if Stack.length ctx.stack = 0 then 
    None
  else if Hash_set.exists (Stack.top_exn ctx.stack) ~f:((=) key) then
    Some (List.hd_exn @@ Hashtbl.find_exn ctx.map key)
  else 
    None

(* [remove context name] removes a varable from current scope *)
let remove ctx key =
  match Hashtbl.find ctx.map key with
  | Some (hd :: tl) -> 
    begin match tl with 
      | [] -> Hashtbl.remove ctx.map key
      | tl -> Hashtbl.set ctx.map ~key ~data:tl
    end;
    ignore @@ Stack.find ctx.stack ~f:(fun set ->
      match Hash_set.find set ~f:((=)key) with
      | Some _ ->  
        Hash_set.remove set key;
        true
      | None -> false);
  | _ -> ()

(** [dump context] dumps [context] vtable to debug output  *)
let dump ctx =
  let open Util in
  dbg "=== == - CONTEXT DUMP - == ===";
  dbg "-> Filename: %s" ctx.filename;
  dbg "-> Keys:";

  let sortf (xa, xb) (ya, yb) = 
    compare (xb, xa) (yb, ya) 
  in
  let ind x = 
    String.make (x * 3) ' ' 
  in
  let rec prn ?(depth=1) ctx = 
    let prn_assignable ass = 
      match ass with
      | Namespace.Var _    -> sprintf "(*var*)", ""
      | Namespace.Func _   -> sprintf "(*fun*)", ""
      | Namespace.Type _   -> sprintf "(*typ*)", ""
      | Namespace.Import ctx -> 
        sprintf "(*imp*)", 
        " ->\n" ^ (prn ctx ~depth:(depth+1))
    in
    let sorted = 
      Hashtbl.to_alist ctx |>
      List.map ~f:(fun (a, b) -> (a, List.hd_exn b)) |>
      List.sort ~compare:sortf
    in
    String.concat ~sep:"\n" @@ List.map sorted ~f:(fun (key, data) -> 
      let pre, pos = prn_assignable (fst data) in
      sprintf "%s%s %s %s" (ind depth) pre key pos)
  in 
  dbg "%s" (prn ctx.map)
