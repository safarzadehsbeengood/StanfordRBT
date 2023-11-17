#include "RedBlackTree.h"
#include <iostream>
#include <iomanip>
#include <stdexcept>
using namespace std;

RedBlackTree::~RedBlackTree() {
  /* Deallocates all memory used by the tree. This algorithm uses O(1) auxiliary
   * storage space and is not recursive. It's due to a friend of mine, Leo
   * Shamis, who mentioned it to me after I told him that I wasn't sure whether
   * such an algorithm even existed!
   *
   * The algorithm is very simple. If the root has no left child, we just delete
   * it and move on. Otherwise, it has a left child, so we do a right rotation
   * to reduce the size of the right subtree a bit.
   */
  while (root != nullptr) {
    /* Case 1: The root has no left child. */
    if (root->left == nullptr) {
      Node* next = root->right;
      delete root;
      root = next;
    }
    /* Case 2: There is a left child, so do a right rotation. */
    else {
      /* We could go through the rotateWithParent function, but that's
       * unnecessary given that we're destroying the tree.
       */
      Node* leftChild = root->left;
      root->left = leftChild->right;
      leftChild->right = root;
      root = leftChild;
    }
  }
}

/* Standard tree search. */
bool RedBlackTree::contains(int key) const {
  Node* curr = root;
  while (curr != nullptr) {
    if      (key == curr->key)   return true;
    else if (key <  curr->key)   curr = curr->left;
    else /*  key >  curr->key */ curr = curr->right;
  }
  return false;
}

/* Insertion works in two phases. First, we do the regular BST insertion. Then,
 * we apply fixup rules to correct the tree
 */
bool RedBlackTree::insert(int key) {
  /* Insert the key and get a pointer to the new node. The insertion function
   * returns null if the key already existed.
   */
  Node* node = insertKey(key);
  if (node == nullptr) return false;
  
  /* Now, perform fixup logic to restore the red/black properties. */
  fixupFrom(node);
  return true;
}

/* Inserts the given key into the red/black tree, returning either a pointer to
 * the newly-created node holding it or a null pointer if the key already was
 * present in the tree.
 */
RedBlackTree::Node* RedBlackTree::insertKey(int key) {
  /* Step one: Find the insertion point. */
  Node* prev = nullptr;
  Node* curr = root;
  
  while (curr != nullptr) {
    prev = curr;
    
    if      (key == curr->key)   return nullptr;       // Already present
    else if (key <  curr->key)   curr = curr->left;
    else /*  key >  curr->key */ curr = curr->right;
    prev->size++; // Going to a subtree, so increment previous node's size
  }
  
  /* Step two: Do the actual insertion. */
  Node* node   = new Node;
  node->key    = key;
  node->color  = Color::BLACK; // Default to black, can change later.
  node->left   = node->right = nullptr; // No children
  node->parent = prev; // Parent is the last node we saw
  
  /* Step three: Wire this node into the tree. */
  if (prev == nullptr) {
    root = node;
  } else if (key < prev->key) {
    prev->left = node;
  } else /*  key > prev->key */ {
    prev->right = node;
  }

  node->size = 1; // Size of new node is 1
  
  return node;
}

/* Applies the fixup rules to restore the red/black tree invariants. */
void RedBlackTree::fixupFrom(Node* node) {
  while (true) {
    /* If the node is the root, then there's nothing to do. */
    if (node->parent == nullptr) break;
  
    /* For simplicity, get pointers to our parent, sibling, aunt, and grandparent.
     * These are the nodes marked in this diagram:
     *
     *           G
     *          / \
     *         P   A
     *        / \
     *       N   S
     *
     * Here, N is the node itself.
     */       
    Node* parent = node->parent;
    Node* grandparent = parent->parent;
    
    /* The SIBLING of a node is the other child of its parent. Its AUNT is its
     * parent's sibling.
     */
    Node* sibling = siblingOf(node);
    Node* aunt    = siblingOf(parent);
    
    /* If the parent corresponds to a node with one key in the 2-3-4 tree (that
     * is, the parent is a black node), then via the isometry we add ourselves
     * to that node by coloring ourselves red. At that point, we're done.
     *
     * To see if our parent corresponds to a node with one key in the 2-3-4
     * tree, we need to check that
     *
     *   1. the parent is black (if it's red, we're in part of a larger node), and
     *   2. the parent has no red children (if it does, then it's part of a larger
     *      node).
     *
     * To do this, we'll find our sibling node (the node across from us under our
     * parent) and confirm that it's not red.
     */
    if (parent->color == Color::BLACK && (sibling == nullptr || sibling->color == Color::BLACK)) {
      //cout << "Insert into 2-node." << endl;
      node->color = Color::RED;
      break;
    }
    
    /* If the parent is part of a node with two keys in the 2-3-4 tree, add
     * ourselves to that node. There are several cases to consider here, and
     * they're all symmetric. A node with two keys has one of these shapes,
     * with all possible insertion points marked with an I:
     *
     *          B              B
     *         / \            / \
     *        R   I          I   R
     *       / \                / \
     *      I   I              I   I
     *
     * The commonality is that we would be in one of two cases:
     *
     *    1. We have a black parent and a red sibling.
     *    2. We have a red parent and a black aunt.
     *
     * These two cases function differently. If we're in case 1, we just color
     * ourselves red:
     *
     *         B             B
     *        / \    -->    / \
     *       N   R         R   R
     *
     * Fun fact - this subcase of inserting into a 3-node can be combined with
     * the logic for inserting into a 2-node. Do you see why?
     */
    if (parent->color == Color::BLACK && sibling != nullptr && sibling->color == Color::RED) {
      //cout << "Insert into 3-node, black parent." << endl;
      node->color = Color::RED;
      break;
    } 
     
    /* That takes us to the second option. */
    if (parent->color == Color::RED && (aunt == nullptr || aunt->color == Color::BLACK)) {
      /* There are two subcases here, which correspond to the relative ordering
       * at which the node to insert appears relative to the two other nodes in
       * the 3-node. The first option is the "zig zag" case:
       *
       *       B                   B                   N                B
       *      / \                 / \                 / \              / \
       *     R   B   --->        N   B    --->       R   B    --->    R   R
       *      \     rotate      /        rotate           \  recolor       \
       *       N   N with R    R        N with B           B                B
       * 
       * To see whether we're in this case, we have to see whether the orientation
       * of the parent/child and grandparent/parent relations are reversed.
       */
      if ((node == parent->left) != (parent == grandparent->left)) {
        //cout << "Insert into 3-node, zig-zag." << endl;
        rotateWithParent(node);
        rotateWithParent(node);
        grandparent->color = Color::RED;
      }
      
      /* The other option is the "zig-zig" case:
       *
       *      B               R                  B
       *     / \             / \                / \
       *    R   B   --->    N   B      --->    R   R
       *   /       rotate        \    recolor       \
       *  N       R with B        B                  B
       */
      else {
        //cout << "Insert into 3-node, zig-zig." << endl;
        rotateWithParent(parent);
        parent->color      = Color::BLACK;
        node->color        = Color::RED;
        grandparent->color = Color::RED;
      }
      
      /* Both cases are terminal; we've inserted into a 3-node. */
      break;
    }
    
    /* Otherwise, we are inserting into a 4-node. There are several orientations
     * possible here, but with mirroring excluded there are basically two unique
     * insertion points
     *
     *          B              B
     *        /   \          /   \
     *       R     R        R     R
     *      /                \
     *     I                  I
     *
     * We are splitting a node with four keys into a node with two keys, a node
     * with one key, and then kicking one key higher up. This can be done purely
     * by recoloring the nodes and continuing the search from a starred node that
     * is colored black beforehand:
     *
     *          B              B
     *        /   \          /   \
     *       R     R        R     R
     *      /                \
     *     I                  I
     *         vvv            vvvv
     *
     *          *              *
     *        /   \          /   \
     *       B     B        B     B
     *      /                \
     *     R                  R
     *
     * In other words, we just flip the colors of the nodes and propagate the
     * search upward from the grandparent.
     */
    //cout << "Insert into 4-node, zig-zag." << endl;
    parent->color = Color::BLACK;
    aunt->color   = Color::BLACK;
    node->color   = Color::RED;
    
    node = grandparent;
  }
}

/* Standard rotation logic. We just have to remember to adjust the root and
 * parent pointers as needed.
 */
void RedBlackTree::rotateWithParent(Node* node) {
  /* If we're the root, something terrible has happened. */
  if (node->parent == nullptr) {
    throw runtime_error("Rotating node with no parent?");
  }
  
  /* Step 1: Do the logic to "locally" rotate the nodes. This repositions the
   * node, its parent, and the middle child. However, it leaves the parent
   * pointers of these nodes unmodified; we'll handle that later.
   */
  Node* child;

  if (node == node->parent->left) {
    /* Rotate right. */
    child = node->right;
    node->right = node->parent;
    node->parent->left = child;

    /* Update sizes. */
    node->size = node->parent->size;
    node->parent->size = (node->parent->right != nullptr ? node->right->size : 0) + (node->parent->left != nullptr ? node->parent->left->size : 0) + 1;
  } else {
    /* Rotate left. */
    child = node->left;
    node->left = node->parent;
    node->parent->right = child;

    /* Update sizes. */
    node->size = node->parent->size;
    node->parent->size = (node->parent->left != nullptr ? node->parent->left->size : 0) + (node->parent->right != nullptr ? node->parent->right->size : 0) + 1;
  }
  
  /* Step 2: Make the node's grandparent now point at it. */
  Node* grandparent = node->parent->parent;
  
  if (grandparent != nullptr) {
    if (grandparent->left == node->parent) {
      grandparent->left = node;
    }
    else {
      grandparent->right = node;
    }

  } else {
    root = node;
  }
  
  /* Step 3: Update parent pointers.
   *
   *  1. The child node that got swapped needs its parent updated.
   *  2. The node we rotated now has a new parent.
   *  3. The node's old parent now points to the node we rotated.
   *
   * We have to be super careful about this, though, because some of these
   * nodes might not exist and we need to not lose any pointers.
   */
  if (child != nullptr) child->parent = node->parent;

  Node* oldParent = node->parent;
  node->parent = oldParent->parent;
  oldParent->parent = node;
}

/* Returns the sibling of a node, the other child of its parent. */
RedBlackTree::Node* RedBlackTree::siblingOf(Node* node) {
  Node* parent = node->parent;
  
  /* A node with no parent has no sibling. */
  if (parent == nullptr) return nullptr;
  
  /* Otherwise, return the opposite child. */
  return node == parent->left? parent->right : parent->left;
}

/* Rank operation. */
size_t RedBlackTree::rankOf(int key) const {
  return rankOfHelper(root, key);
}

size_t RedBlackTree::rankOfHelper(Node* root, int key) const {
  if (root == nullptr) {
    return 0;
  } else if (key < root->key) {
    // If the key is less than the current node's key, go to the left subtree.
    return rankOfHelper(root->left, key);
  } else if (key > root->key) {
    // If the key is greater than the current node's key, go to the right subtree.
    // Add 1 to account for the current node and the nodes in the left subtree.
    return 1 + (root->left ? root->left->size : 0) + rankOfHelper(root->right, key);
  } else {
    // If the current node's key matches the key, return the count of nodes
    // in the left subtree (nodes with keys smaller than the current node).
    return root->left ? root->left->size : 0;
  }
}


/* Select operation. */
int RedBlackTree::select(size_t rank) const {
  return selectHelper(this->root, rank);
}

int RedBlackTree::selectHelper(Node* root, size_t rank) const {
  if (root == nullptr) {
    return 0;
  } 
  size_t leftcount;
  if (root->left == nullptr) {
    leftcount = 0;
  } else {
    leftcount = root->left->size;
  }

  if (rank < leftcount) {
    return selectHelper(root->left, rank);
  } else if (rank == leftcount) {
    return root->key;
  } else {
    return selectHelper(root->right, rank - leftcount - 1);
  }
}



/* Prints debugging information. This is just to make testing a bit easier. */
void RedBlackTree::printDebugInfo() const {
  printDebugInfoRec(root, 0);
  cout << flush;
}

/* Prints information about this node and its left and right subtrees.
 *
 * Optional TODO: Edit this function to print out additional debugging
 * information for testing.
 */
void RedBlackTree::printDebugInfoRec(Node* root, unsigned indent) const {
  if (root == nullptr) {
    cout << setw(indent) << "" << "null" << '\n';
  } else {
    cout << setw(indent) << "" << "\x1B[32mNode       \x1B[0m" << root << '\n';
    string color = colorToString(root->color);
    if (color == "red") cout << setw(indent) << "" << "Color:     \x1B[31m" << color << "\x1B[0m\n";
    else cout << setw(indent) << "" << "Color:     " << color << '\n';
    cout << setw(indent) << "" << "Key:       " << root->key << '\n';
    cout << setw(indent) << "" << "Size:      " << root->size << '\n';
    cout << setw(indent) << "" << "Left Child:" << '\n';
    printDebugInfoRec(root->left,  indent + 4);
    cout << setw(indent) << "" << "Right Child:" << '\n';
    printDebugInfoRec(root->right, indent + 4);
  }
}
