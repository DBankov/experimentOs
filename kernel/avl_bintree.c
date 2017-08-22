
#include <bintree.h>

#define ALLOWED_IMBALANCE 1

static inline bintree_node *
obj_to_bintree_node(void *obj, ptrdiff_t offset)
{
   return obj ? (bintree_node *)((char*)obj + offset) : NULL;
}

static inline void *
bintree_node_to_obj(bintree_node *node, ptrdiff_t offset)
{
   return node ? (void *)((char*)node - offset) : NULL;
}

#define OBJTN(o) (obj_to_bintree_node((o), bintree_offset))
#define NTOBJ(n) (bintree_node_to_obj((n), bintree_offset))

#define LEFT_OF(obj) ( OBJTN((obj))->left_obj )
#define RIGHT_OF(obj) ( OBJTN((obj))->right_obj )
#define HEIGHT(obj) ((obj) ? OBJTN((obj))->height : -1)

static inline void
update_height(bintree_node *node, ptrdiff_t bintree_offset)
{
   node->height = MAX(HEIGHT(node->left_obj), HEIGHT(node->right_obj)) + 1;
}

#define UPDATE_HEIGHT(n) update_height((n), bintree_offset)



/*
 * rotate the left child of *obj_ref clock-wise
 *
 *         (n)                  (nl)
 *         /  \                 /  \
 *       (nl) (nr)   ==>    (nll)  (n)
 *       /  \                     /   \
 *    (nll) (nlr)               (nlr) (nr)
 */

void rotate_cw_left_child(void **obj_ref, ptrdiff_t bintree_offset)
{
   ASSERT(obj_ref != NULL);
   ASSERT(*obj_ref != NULL);

   bintree_node *orig_node = OBJTN(*obj_ref);
   ASSERT(orig_node->left_obj != NULL);

   bintree_node *orig_left_child = OBJTN(orig_node->left_obj);
   *obj_ref = orig_node->left_obj;
   orig_node->left_obj = orig_left_child->right_obj;
   OBJTN(*obj_ref)->right_obj = NTOBJ(orig_node);

   UPDATE_HEIGHT(orig_node);
   UPDATE_HEIGHT(orig_left_child);
}

/*
 * rotate the right child of *obj_ref counterclock-wise
 */

void rotate_ccw_right_child(void **obj_ref, ptrdiff_t bintree_offset)
{
   ASSERT(obj_ref != NULL);
   ASSERT(*obj_ref != NULL);

   bintree_node *orig_node = OBJTN(*obj_ref);
   ASSERT(orig_node->right_obj != NULL);

   bintree_node *orig_right_child = OBJTN(orig_node->right_obj);
   *obj_ref = orig_node->right_obj;
   orig_node->right_obj = orig_right_child->left_obj;
   OBJTN(*obj_ref)->left_obj = NTOBJ(orig_node);

   UPDATE_HEIGHT(orig_node);
   UPDATE_HEIGHT(orig_right_child);
}


static void validate_bst(void *obj, ptrdiff_t bintree_offset, cmpfun_ptr cmp)
{
   if (!obj) return;

   if (LEFT_OF(obj)) {
      ASSERT(cmp(LEFT_OF(obj), obj) < 0);
   }

   if (RIGHT_OF(obj)) {
      ASSERT(cmp(RIGHT_OF(obj), obj) > 0);
   }
}

#define ROTATE_CW_LEFT_CHILD(obj) (rotate_cw_left_child((obj), bintree_offset))
#define ROTATE_CCW_RIGHT_CHILD(obj) (rotate_ccw_right_child((obj), bintree_offset))
#define VALIDATE_BST(obj) (validate_bst((obj), bintree_offset, cmp))
#define BALANCE(obj) (balance((obj), bintree_offset))

static void balance(void **obj_ref, ptrdiff_t bintree_offset)
{
   ASSERT(obj_ref != NULL);

   if (*obj_ref == NULL)
      return;

   void *left_obj = LEFT_OF(*obj_ref);
   void *right_obj = RIGHT_OF(*obj_ref);

   int bf = HEIGHT(left_obj) - HEIGHT(right_obj);

   if (bf > ALLOWED_IMBALANCE) {

      if (HEIGHT(LEFT_OF(left_obj)) >= HEIGHT(RIGHT_OF(left_obj))) {
         ROTATE_CW_LEFT_CHILD(obj_ref);
      } else {
         ROTATE_CCW_RIGHT_CHILD(&LEFT_OF(*obj_ref));
         ROTATE_CW_LEFT_CHILD(obj_ref);
      }

   } else if (bf < -ALLOWED_IMBALANCE) {

      if (HEIGHT(RIGHT_OF(right_obj)) >= HEIGHT(LEFT_OF(right_obj))) {
         ROTATE_CCW_RIGHT_CHILD(obj_ref);
      } else {
         ROTATE_CW_LEFT_CHILD(&RIGHT_OF(*obj_ref));
         ROTATE_CCW_RIGHT_CHILD(obj_ref);
      }
   }

   UPDATE_HEIGHT(OBJTN(*obj_ref));
}

bool
bintree_insert_internal(void **root_obj_ref,
                        void *obj,
                        cmpfun_ptr cmp,
                        ptrdiff_t bintree_offset)
{
   /*
    * It will contain the whole reverse path leaf to root objects traversed:
    * that is needed for the balance at the end (it simulates the stack
    * unwinding that happens for recursive implementations).
    */
   void **stack[32] = {0};
   int stack_size = 0;

   while (true) {

      ASSERT(root_obj_ref != NULL);
      ASSERT(*root_obj_ref != NULL);

      bintree_node *root = OBJTN(*root_obj_ref);

      int c = cmp(obj, *root_obj_ref);

      if (c == 0) {
         return false; // such elem already exists.
      }

      if (c < 0) {

         if (!root->left_obj) {
            root->left_obj = obj;
            BALANCE(&root->left_obj);

            DEBUG_ONLY(VALIDATE_BST(root->left_obj));
            break;
         }

         stack[stack_size++] = root_obj_ref;
         root_obj_ref = &root->left_obj;
         continue;
      }

      // case c > 0

      if (!root->right_obj) {
         root->right_obj = obj;
         BALANCE(&root->right_obj);

         DEBUG_ONLY(VALIDATE_BST(root->right_obj));
         break;
      }

      stack[stack_size++] = root_obj_ref;
      root_obj_ref = &root->right_obj;
   }

   stack[stack_size++] = root_obj_ref;

   while (stack_size >= 0) {
      BALANCE(stack[--stack_size]);
   }

   DEBUG_ONLY(VALIDATE_BST(*root_obj_ref));
   return true;
}
