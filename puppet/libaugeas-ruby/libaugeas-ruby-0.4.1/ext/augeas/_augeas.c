/*
 * augeas.c: Ruby bindings for augeas
 *
 * Copyright (C) 2008 Red Hat Inc.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307  USA
 *
 * Author: Bryan Kearney <bkearney@redhat.com>
 */
#include <ruby.h>
#include <augeas.h>

static VALUE c_augeas;

#define StringValueCStrOrNull(v)                \
    NIL_P(v) ? NULL : StringValueCStr(v)

static augeas *aug_handle(VALUE s) {
    augeas *aug;

    Data_Get_Struct(s, struct augeas, aug);
    if (aug == NULL) {
        rb_raise(rb_eSystemCallError, "Failed to retrieve connection");
    }
    return aug;
}

static void augeas_free(augeas *aug) {
    if (aug != NULL)
        aug_close(aug);
}

/*
 * call-seq:
 *   get(PATH) -> String
 *
 * Lookup the value associated with PATH
 */
VALUE augeas_get(VALUE s, VALUE path) {
    augeas *aug = aug_handle(s);
    const char *cpath = StringValueCStr(path);
    const char *value;

    aug_get(aug, cpath, &value);
    if (value != NULL) {
        return rb_str_new(value, strlen(value)) ;
    } else {
        return Qnil;
    }
}

/*
 * call-seq:
 *   exists(PATH) -> boolean
 *
 * Return true if there is an entry for this path, false otherwise
 */
VALUE augeas_exists(VALUE s, VALUE path) {
    augeas *aug = aug_handle(s);
    const char *cpath = StringValueCStr(path);
    int ret = aug_get(aug, cpath, NULL);

    return (ret == 1) ? Qtrue : Qfalse;
}

/*
 * call-seq:
 *   set(PATH, VALUE) -> boolean
 *
 * Set the value associated with PATH to VALUE. VALUE is copied into the
 * internal data structure. Intermediate entries are created if they don't
 * exist.
 */
VALUE augeas_set(VALUE s, VALUE path, VALUE value) {
    augeas *aug = aug_handle(s);
    const char *cpath = StringValueCStr(path) ;
    const char *cvalue = StringValueCStrOrNull(value) ;

    int callValue = aug_set(aug, cpath, cvalue) ;
    VALUE returnValue ;

    if (callValue == 0)
        returnValue = Qtrue ;
    else
        returnValue = Qfalse ;

    return returnValue ;
}

/*
 * call-seq:
 *   setm(BASE, SUB, VALUE) -> boolean
 *
 *  Set multiple nodes in one operation. Find or create a node matching
 *  SUB by interpreting SUB as a path expression relative to each node
 *  matching BASE. SUB may be NULL, in which case all the nodes matching
 *  BASE will be modified.
 */
VALUE augeas_setm(VALUE s, VALUE base, VALUE sub, VALUE value) {
    augeas *aug = aug_handle(s);
    const char *cbase = StringValueCStr(base) ;
    const char *csub = StringValueCStrOrNull(sub) ;
    const char *cvalue = StringValueCStrOrNull(value) ;

    int callValue = aug_setm(aug, cbase, csub, cvalue) ;
    return INT2FIX(callValue);
}

/*
 * call-seq:
 *   insert(PATH, LABEL, BEFORE) -> int
 *
 * Make LABEL a sibling of PATH by inserting it directly before or after PATH.
 * The boolean BEFORE determines if LABEL is inserted before or after PATH.
 */
VALUE augeas_insert(VALUE s, VALUE path, VALUE label, VALUE before) {
    augeas *aug = aug_handle(s);
    const char *cpath = StringValueCStr(path) ;
    const char *clabel = StringValueCStr(label) ;

    int callValue = aug_insert(aug, cpath, clabel, RTEST(before));
    return INT2FIX(callValue) ;
}

/*
 * call-seq:
 *   mv(SRC, DST) -> int
 *
 * Move the node SRC to DST. SRC must match exactly one node in the
 * tree. DST must either match exactly one node in the tree, or may not
 * exist yet. If DST exists already, it and all its descendants are
 * deleted. If DST does not exist yet, it and all its missing ancestors are
 * created.
 */
VALUE augeas_mv(VALUE s, VALUE src, VALUE dst) {
    augeas *aug = aug_handle(s);
    const char *csrc = StringValueCStr(src);
    const char *cdst = StringValueCStr(dst);
    int r = aug_mv(aug, csrc, cdst);

    return INT2FIX(r);
}

/*
 * call-seq:
 *   rm(PATH) -> int
 *
 * Remove path and all its children. Returns the number of entries removed
 */
VALUE augeas_rm(VALUE s, VALUE path, VALUE sibling) {
    augeas *aug = aug_handle(s);
    const char *cpath = StringValueCStr(path) ;

    int callValue = aug_rm(aug, cpath) ;
    return INT2FIX(callValue) ;
}

/*
 * call-seq:
 *       match(PATH) -> an_array
 *
 * Return all the paths that match the path expression PATH as an aray of
 * strings.
 */
VALUE augeas_match(VALUE s, VALUE p) {
    augeas *aug = aug_handle(s);
    const char *path = StringValueCStr(p);
    char **matches = NULL;
    int cnt, i;

    cnt = aug_match(aug, path, &matches) ;
    if (cnt < 0)
        rb_raise(rb_eSystemCallError, "Matching path expression '%s' failed",
                 path);

    VALUE result = rb_ary_new();
    for (i = 0; i < cnt; i++) {
        rb_ary_push(result, rb_str_new(matches[i], strlen(matches[i])));
        free(matches[i]) ;
    }
    free (matches) ;

    return result ;
}

/*
 * call-seq:
 *       save() -> boolean
 *
 * Write all pending changes to disk
 */
VALUE augeas_save(VALUE s) {
    augeas *aug = aug_handle(s);
    int callValue = aug_save(aug) ;
    VALUE returnValue ;

    if (callValue == 0)
        returnValue = Qtrue ;
    else
        returnValue = Qfalse ;

    return returnValue ;
}

/*
 * call-seq:
 *       load() -> boolean
 *
 * Load files from disk according to the transforms under +/augeas/load+
 */
VALUE augeas_load(VALUE s) {
    augeas *aug = aug_handle(s);
    int callValue = aug_load(aug);
    VALUE returnValue ;

    if (callValue == 0)
        returnValue = Qtrue ;
    else
        returnValue = Qfalse ;

    return returnValue ;
}

/*
 * call-seq:
 *   defvar(NAME, EXPR) -> boolean
 *
 * Define a variable NAME whose value is the result of evaluating EXPR. If
 * a variable NAME already exists, its name will be replaced with the
 * result of evaluating EXPR.
 *
 * If EXPR is NULL, the variable NAME will be removed if it is defined.
 *
 */
VALUE augeas_defvar(VALUE s, VALUE name, VALUE expr) {
    augeas *aug = aug_handle(s);
    const char *cname = StringValueCStr(name);
    const char *cexpr = StringValueCStrOrNull(expr);

    int r = aug_defvar(aug, cname, cexpr);

    return (r < 0) ? Qfalse : Qtrue;
}

/*
 * call-seq:
 *   defnode(NAME, EXPR, VALUE) -> boolean
 *
 * Define a variable NAME whose value is the result of evaluating EXPR,
 * which must be non-NULL and evaluate to a nodeset. If a variable NAME
 * already exists, its name will be replaced with the result of evaluating
 * EXPR.
 *
 * If EXPR evaluates to an empty nodeset, a node is created, equivalent to
 * calling AUG_SET(AUG, EXPR, VALUE) and NAME will be the nodeset containing
 * that single node.
 *
 * Returns +false+ if +aug_defnode+ fails, and the number of nodes in the
 * nodeset on success.
 */
VALUE augeas_defnode(VALUE s, VALUE name, VALUE expr, VALUE value) {
    augeas *aug = aug_handle(s);
    const char *cname = StringValueCStr(name);
    const char *cexpr = StringValueCStrOrNull(expr);
    const char *cvalue = StringValueCStrOrNull(value);

    /* FIXME: Figure out a way to return created, maybe accept a block
       that gets run when created == 1 ? */
    int r = aug_defnode(aug, cname, cexpr, cvalue, NULL);

    return (r < 0) ? Qfalse : INT2NUM(r);
}

VALUE augeas_init(VALUE m, VALUE r, VALUE l, VALUE f) {
    unsigned int flags = NUM2UINT(f);
    const char *root = StringValueCStrOrNull(r);
    const char *loadpath = StringValueCStrOrNull(l);
    augeas *aug = NULL;

    aug = aug_init(root, loadpath, flags);
    if (aug == NULL) {
        rb_raise(rb_eSystemCallError, "Failed to initialize Augeas");
    }
    return Data_Wrap_Struct(c_augeas, NULL, augeas_free, aug);
}

VALUE augeas_close (VALUE s) {
    augeas *aug = aug_handle(s);

    aug_close(aug);
    DATA_PTR(s) = NULL;

    return Qnil;
}

static void hash_set(VALUE hash, const char *sym, VALUE v) {
    rb_hash_aset(hash, ID2SYM(rb_intern(sym)), v);
}

/*
 * call-seq:
 *   error -> HASH
 *
 * Retrieve details about the last error encountered and return those
 * details in a HASH with the following entries:
 * - :code    error code from +aug_error+
 * - :message error message from +aug_error_message+
 * - :minor   minor error message from +aug_minor_error_message+
 * - :details error details from +aug_error_details+
 */
VALUE augeas_error(VALUE s) {
    augeas *aug = aug_handle(s);
    int code;
    const char *msg;
    VALUE result;

    result = rb_hash_new();

    code = aug_error(aug);
    hash_set(result, "code", INT2NUM(code));

    msg = aug_error_message(aug);
    if (msg != NULL)
        hash_set(result, "message", rb_str_new2(msg));

    msg = aug_error_minor_message(aug);
    if (msg != NULL)
        hash_set(result, "minor", rb_str_new2(msg));

    msg = aug_error_details(aug);
    if (msg != NULL)
        hash_set(result, "details", rb_str_new2(msg));

    return result;
}

static void hash_set_range(VALUE hash, const char *sym,
                           unsigned int start, unsigned int end) {
    VALUE r;

    r = rb_range_new(INT2NUM(start), INT2NUM(end), 0);
    hash_set(hash, sym, r);
}

VALUE augeas_span(VALUE s, VALUE path) {
    augeas *aug = aug_handle(s);
    char *cpath = StringValueCStr(path);
    char *filename = NULL;
    unsigned int label_start, label_end, value_start, value_end,
        span_start, span_end;
    int r;
    VALUE result;

    r = aug_span(aug, cpath,
                 &filename,
                 &label_start, &label_end,
                 &value_start, &value_end,
                 &span_start, &span_end);

    result = rb_hash_new();

    if (r == 0) {
        hash_set(result, "filename", rb_str_new2(filename));
        hash_set_range(result, "label", label_start, label_end);
        hash_set_range(result, "value", value_start, value_end);
        hash_set_range(result, "span", span_start, span_end);
    }

    free(filename);

    return result;
}

void Init__augeas() {

    /* Define the ruby class */
    c_augeas = rb_define_class("Augeas", rb_cObject) ;

    /* Constants for enum aug_flags */
#define DEF_AUG_FLAG(name) \
    rb_define_const(c_augeas, #name, INT2NUM(AUG_##name))
    DEF_AUG_FLAG(NONE);
    DEF_AUG_FLAG(SAVE_BACKUP);
    DEF_AUG_FLAG(SAVE_NEWFILE);
    DEF_AUG_FLAG(TYPE_CHECK);
    DEF_AUG_FLAG(NO_STDINC);
    DEF_AUG_FLAG(SAVE_NOOP);
    DEF_AUG_FLAG(NO_LOAD);
    DEF_AUG_FLAG(NO_MODL_AUTOLOAD);
    DEF_AUG_FLAG(ENABLE_SPAN);
#undef DEF_AUG_FLAG

    /* Constants for enum aug_errcode_t */
#define DEF_AUG_ERR(name) \
    rb_define_const(c_augeas, #name, INT2NUM(AUG_##name))
    DEF_AUG_ERR(NOERROR);
    DEF_AUG_ERR(ENOMEM);
    DEF_AUG_ERR(EINTERNAL);
    DEF_AUG_ERR(EPATHX);
    DEF_AUG_ERR(ENOMATCH);
    DEF_AUG_ERR(EMMATCH);
    DEF_AUG_ERR(ESYNTAX);
    DEF_AUG_ERR(ENOLENS);
    DEF_AUG_ERR(EMXFM);
#undef DEF_AUG_ERR

    /* Define the methods */
    rb_define_singleton_method(c_augeas, "open3", augeas_init, 3);
    rb_define_method(c_augeas, "defvar", augeas_defvar, 2);
    rb_define_method(c_augeas, "defnode", augeas_defnode, 3);
    rb_define_method(c_augeas, "get", augeas_get, 1);
    rb_define_method(c_augeas, "exists", augeas_exists, 1);
    rb_define_method(c_augeas, "insert", augeas_insert, 3);
    rb_define_method(c_augeas, "mv", augeas_mv, 2);
    rb_define_method(c_augeas, "rm", augeas_rm, 1);
    rb_define_method(c_augeas, "match", augeas_match, 1);
    rb_define_method(c_augeas, "save", augeas_save, 0);
    rb_define_method(c_augeas, "load", augeas_load, 0);
    rb_define_method(c_augeas, "set_internal", augeas_set, 2);
    rb_define_method(c_augeas, "setm", augeas_setm, 3);
    rb_define_method(c_augeas, "close", augeas_close, 0);
    rb_define_method(c_augeas, "error", augeas_error, 0);
    rb_define_method(c_augeas, "span", augeas_span, 1);
}

/*
 * Local variables:
 *  indent-tabs-mode: nil
 *  c-indent-level: 4
 *  c-basic-offset: 4
 *  tab-width: 4
 * End:
 */
