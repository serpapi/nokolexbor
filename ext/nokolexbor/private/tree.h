#ifndef XML_TREE_H_PRIVATE__
#define XML_TREE_H_PRIVATE__

/*
 * Internal variable indicating if a callback has been registered for
 * node creation/destruction. It avoids spending a lot of time in locking
 * function while checking if the callback exists.
 */
XML_HIDDEN extern int
__xmlRegisterCallbacks;

XML_HIDDEN lxb_dom_node_t_ptr
xmlStaticCopyNode(lxb_dom_node_t_ptr node, lxb_dom_document_t_ptr doc, lxb_dom_node_t_ptr parent,
                  int extended);
XML_HIDDEN lxb_dom_node_t_ptr
xmlStaticCopyNodeList(lxb_dom_node_t_ptr node, lxb_dom_document_t_ptr doc, lxb_dom_node_t_ptr parent);

#endif /* XML_TREE_H_PRIVATE__ */
