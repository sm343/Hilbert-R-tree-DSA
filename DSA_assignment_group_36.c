#include <math.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>

#define MAX_ENTRIES 4
#define MIN_ENTRIES 2

/* -----------------------------------------------STRUCTURE-----------------------------------------------------------
 */
typedef struct rtree Rtree;
typedef struct node Node;
typedef struct point Point;
typedef struct rectangle Rect;
typedef struct nodeEle NodeEle;
typedef struct splitResult SplitResult;

// Assuming the coordinates to be integers

// This struct represents the cartesian coordinates of a point
struct point
{
    int x;
    int y;
};

// A rectangle has two corner points - TopLeft & BottomRight (Assuming the sides to be parallel to x and y axes)
struct rectangle
{
    Point topRight;
    Point bottomLeft;
};

// Element of a node that has an MBR and child node
struct nodeEle
{
    Rect mbr;
    Node *child;
    Node *container;  // node which encapsulates the current MBR
};

// Node contains multiple node elements
struct node
{
    bool isLeaf;         // leaf or non-leaf
    int count;           // no of node elements present
    NodeEle **elements;  // array of node elements present within node
    NodeEle *parent;     // parent element of node
};

// Tree structure having root node
struct rtree
{
    Node *root;
};

// Temporary struct used to help with node splitting to propagate data up the tree
struct splitResult
{
    NodeEle *parent;  // parent of the node before splitting
    // leaf1 and leaf2 are the two splits of node
    Node *leaf1;
    Node *leaf2;
};

/* -------------------------FUNCTION DEFINITIONS--------------------------- */
NodeEle *createNodeEle(Node *container, Point topRight, Point bottomLeft);
Node *createNode(NodeEle *parent, bool isLeaf);
Rtree *createRtree();

void traversal(Node *root, bool isInit);

int calculateAreaOfRectangle(Rect rec);
Rect createMBR(Rect rect1, Rect rect2);
int calcAreaEnlargement(Rect rectCont, Rect rectChild);
void createNodeParent(Node *node);
void updateParent(NodeEle *n, Node *n1, Node *n2);

NodeEle *chooseSubTree(Node *n, Rect r);
Node *ChooseLeaf(Rtree *r, Rect r1);

void pickSeeds(Node *node, Node *node1, Node *node2);
void pickNext(Node *node, Node *node1, Node *node2);
SplitResult *nodeSplit(Node *node);
bool isPresent(NodeEle **e1, int s, NodeEle *e2);

SplitResult *adjustTree(SplitResult *split);

bool isOverlap(Rect r, Rect mbr);
void search(Node *searchNode, Rect searchRect);

void insert(Rtree *r, Point p1, Point p2);
/* --------------------------------------------GENERATING FUNCTIONS---------------------------------------------------
 */
// create node element
NodeEle *createNodeEle(Node *container, Point topRight, Point bottomLeft)
{
    NodeEle *nodeEle = (NodeEle *)malloc(sizeof(NodeEle));
    nodeEle->container = container;
    nodeEle->child = NULL;
    nodeEle->mbr.bottomLeft = bottomLeft;
    nodeEle->mbr.topRight = topRight;
    return nodeEle;
}
// create node
Node *createNode(NodeEle *parent, bool isLeaf)
{
    Node *node = (Node *)malloc(sizeof(Node));
    node->isLeaf = isLeaf;
    node->count = 0;
    // MAX_ENTRIES + 1 to ensure space for 5th elements right before splitting
    node->elements = (NodeEle **)malloc((MAX_ENTRIES + 1) * sizeof(NodeEle *));
    node->parent = parent;
    return node;  // returning the node
}
// create tree with empty root
Rtree *createRtree()
{
    Rtree *rtree = (Rtree *)malloc(sizeof(Rtree));
    rtree->root = createNode(NULL, true);
    return rtree;
}

/* ----------------------------------------------PREORDER TRAVERSAL----------------------------------------------------
 */
// defining preorder - first, list all the current node elements -> then, traverse all the children in the same manner
void traversal(Node *root, bool isInit)
{
    if (root == NULL) return;
    Rect mbr;
    mbr = root->elements[0]->mbr;
    // Calculate the tree's MBR by repeatedly checking max and min value of container of previous MBRs and current MBR
    if (isInit)  // only for 1st level since traversal is recursive
    {
        for (int i = 1; i < root->count; i++)
        {
            mbr = createMBR(mbr, root->elements[i]->mbr);
        }
        printf("Tree MBR: (%d, %d) -> (%d, %d)", mbr.bottomLeft.x, mbr.bottomLeft.y, mbr.topRight.x, mbr.topRight.y);
    }

    if (root->isLeaf)
        printf("\nLeaf Node: ");
    else if (root->parent == NULL)
        printf("\nRoot Node: ");
    else
        printf("\nInternal Node: ");

    // iterating through all elements of the root
    for (int i = 0; i < root->count; i++)
    {
        Rect rect = root->elements[i]->mbr;
        // Leaf node condition
        if (root->isLeaf)
        {
            if (rect.topRight.x == rect.bottomLeft.x && rect.topRight.y == rect.bottomLeft.y)
            {
                printf("(%d, %d)", rect.topRight.x, rect.topRight.y);
            }
            else
            {
                printf("(%d, %d) -> (%d, %d)", rect.bottomLeft.x, rect.bottomLeft.y, rect.topRight.x, rect.topRight.y);
            }
        }
        // Root node condition
        else if (root->parent == NULL)
        {
            printf("(%d, %d) -> (%d, %d)", rect.bottomLeft.x, rect.bottomLeft.y, rect.topRight.x, rect.topRight.y);
        }
        // Internal node condition
        else
        {
            printf("(%d, %d) -> (%d, %d)", rect.bottomLeft.x, rect.bottomLeft.y, rect.topRight.x, rect.topRight.y);
        }
        if (i < root->count - 1) printf(", ");
    }

    // traverse through all the children of current node
    for (int i = 0; i < root->count; i++)
    {
        if (!root->isLeaf) traversal(root->elements[i]->child, false);
    }
    if (isInit) printf("\n");
}

/*---------------------------HELPERS---------------------------------------------------- */

// create an MBR for two rectangles
Rect createMBR(Rect rect1, Rect rect2)
{
    Rect rect;
    rect.topRight.x = fmax(rect1.topRight.x, rect2.topRight.x);
    rect.topRight.y = fmax(rect1.topRight.y, rect2.topRight.y);
    rect.bottomLeft.x = fmin(rect1.bottomLeft.x, rect2.bottomLeft.x);
    rect.bottomLeft.y = fmin(rect1.bottomLeft.y, rect2.bottomLeft.y);

    return rect;
}

// area of a rectangle
int calculateAreaOfRectangle(Rect rect)
{
    int height = abs(rect.topRight.x - rect.bottomLeft.x);
    int width = abs(rect.topRight.y - rect.bottomLeft.y);
    return height * width;
}

// enlargement area for a rectangle to accomodate another rectangle
int calcAreaEnlargement(Rect rectCont, Rect rectChild)
{
    Rect enlargedRect = createMBR(rectCont, rectChild);
    return calculateAreaOfRectangle(enlargedRect) - calculateAreaOfRectangle(rectCont);
}

// Create a parent Node_ele (MBR) for node.
void createNodeParent(Node *node)
{
    Rect mbr;
    NodeEle *oldParent = node->parent;
    mbr = node->elements[0]->mbr;

    // Calculate the parent's MBR by repeatedly checking max and min value of container of previous MBRs and current MBR
    for (int i = 1; i < node->count; i++)
    {
        mbr = createMBR(mbr, node->elements[i]->mbr);
    }
    NodeEle *parent = createNodeEle(NULL, mbr.topRight, mbr.bottomLeft);

    node->parent = parent;
    parent->child = node;
    parent->mbr = mbr;
    if (oldParent != NULL)
    {
        node->parent->container = oldParent->container;
        free(oldParent);
    }
}

// Update node1 and node2 parent MBRs in container of parent MBR
void updateParent(NodeEle *parent, Node *node1, Node *node2)
{
    Node *parentNode = parent->container;
    int ele;

    // Find the index at which `parent` MBR is present
    for (int i = 0; i < parentNode->count; i++)
    {
        if (parentNode->elements[i] == parent)
        {
            ele = i;
            break;
        }
    }

    parentNode->elements[ele] = node1->parent;
    node1->parent->container = parentNode;

    // If node did not split
    if (node1 != node2)
    {
        parentNode->elements[parentNode->count++] = node2->parent;
        node2->parent->container = parentNode;
        // createNodeParent already frees parent in case of no split
        free(parent);
    }
}

// checks for an overlap between the rectangle and the MBR in a node
// largest of all min values and smallest of all max values should form
// a valid rectangle
bool isOverlap(Rect r, Rect mbr)
{
    int xMin = fmax(r.bottomLeft.x, mbr.bottomLeft.x);
    int xMax = fmin(r.topRight.x, mbr.topRight.x);
    int yMin = fmax(r.bottomLeft.y, mbr.bottomLeft.y);
    int yMax = fmin(r.topRight.y, mbr.topRight.y);
    if (xMin <= xMax && yMin <= yMax) return true;
    return false;
}

/*-------------------------INSERT CODE---------------------------------------------------- */

/* CHOOSE LEAF */

// choose the appropriate subtree where new rectangle can be added
NodeEle *chooseSubTree(Node *node, Rect rectAdd)
{
    // Init variables
    int ele = 0;
    int areaMin = calculateAreaOfRectangle(node->elements[0]->mbr);          // to store min area of rectangles
    int areaEnlarge = calcAreaEnlargement(node->elements[0]->mbr, rectAdd);  // enlarged area

    // Check for maximum enlargement
    for (int i = 1; i < node->count; i++)
    {
        if (node->elements[i] != NULL)
        {
            int area = calculateAreaOfRectangle(node->elements[i]->mbr);
            int areaE = calcAreaEnlargement(node->elements[i]->mbr, rectAdd);

            // min area enlargement
            if (areaE < areaEnlarge)
            {
                areaMin = area;
                areaEnlarge = areaE;
                ele = i;
            }
            else if (areaE == areaEnlarge)
            {
                // if enlargement is same as min, we choose one with minimum area
                if (area < areaMin)
                {
                    areaMin = area;
                    areaEnlarge = areaE;
                    ele = i;
                }
            }
        }
        else
        {
            break;
        }
    }

    return node->elements[ele];  // subtree where rectangle is to be added
}

// Main choose leaf function
Node *ChooseLeaf(Rtree *tree, Rect rectAdd)
{
    Node *node = tree->root;
    while (!node->isLeaf)  // running loop until leaf is reached
    {
        node = chooseSubTree(node, rectAdd)->child;  // descends down to the leaf node
    }

    return node;  // correct leaf node
}

// identify the presence of a rectangle in a node
bool isPresent(NodeEle **ele, int size, NodeEle *searchElem)
{
    Rect mbr;
    for (int i = 0; i < size; i++)
    {
        mbr = ele[i]->mbr;
        if (ele[i] == searchElem) return true;
    }
    return false;
}

/* SPLIT NODE */

// node1 and 2 are the splitted nodes, choose first elements to be inserted in both of them
// using pickseed function
void pickSeeds(Node *node, Node *node1, Node *node2)
{
    int maxArea, area, area1, area2;
    int elem1, elem2;
    Rect rect, rect1, rect2;
    rect1 = node->elements[0]->mbr;  // MBR of 1st node_ele and 2nd node_ele are considered first
    rect2 = node->elements[1]->mbr;

    rect = createMBR(rect1, rect2);  // MBR of the above two considered rect1 and rect2

    area = calculateAreaOfRectangle(rect);  // calculating the area of rect,rect1 and rect2
    area1 = calculateAreaOfRectangle(rect1);
    area2 = calculateAreaOfRectangle(rect2);
    maxArea = area - area1 - area2;
    elem1 = 0;
    elem2 = 1;

    // traverse every possible pair of node elements
    for (int i = 0; i < node->count; i++)
    {
        for (int j = i + 1; j < node->count; j++)
        {
            rect1 = node->elements[i]->mbr;
            rect2 = node->elements[j]->mbr;

            rect = createMBR(rect1, rect2);  // MBR bounding 2 rects

            area = calculateAreaOfRectangle(rect);
            area1 = calculateAreaOfRectangle(rect1);
            area2 = calculateAreaOfRectangle(rect2);

            // calculating the most wasteful area -> insert both rectangles in separate nodes
            if (maxArea < area - area1 - area2)
            {
                // maximizing area-area1-area2 to get the farthest two objects
                // to consider while picking the seeds into two different nodes
                maxArea = area - area1 - area2;
                elem1 = i;  // storing the values of elem1 ad elem2
                elem2 = j;
            }
        }
    }

    node1->elements[0] = node->elements[elem1];  // assigning the elements of node1 and node2
    node2->elements[0] = node->elements[elem2];  // using the previously found values of elem1 and elem2
    node->elements[elem1]->container = node1;    // now these new node_ele are assigned their container nodes.
    node->elements[elem2]->container = node2;
    node1->count = 1;  // increasing count of the nodes by 1
    node2->count = 1;
}

// choose the node where a rect can be inserted after picking initial seeds
void pickNext(Node *node, Node *node1, Node *node2)
{
    int maxDiff = 0, diff, diff1, diff2, idx;  // defining and initializing variables
    int d1, d2;
    int area1 = calculateAreaOfRectangle(node1->parent->mbr);
    int area2 = calculateAreaOfRectangle(node2->parent->mbr);
    bool setFlag = false;  // Ensure that final variables are set for atleast one node

    for (int i = 0; i < node->count; i++)
    {
        // checking if elements in the node are already present or not using the isPresent function

        if (!isPresent(node1->elements, node1->count, node->elements[i]) &&
            !isPresent(node2->elements, node2->count, node->elements[i]))
        {
            // calculate enlargements in both nodes MBR for each rectangle that is not alloted to a node
            d1 = calcAreaEnlargement(node1->parent->mbr, node->elements[i]->mbr);
            d2 = calcAreaEnlargement(node2->parent->mbr, node->elements[i]->mbr);
            diff = abs(d1 - d2);

            // prioritise the rectangle with max diff in enlargements
            if (setFlag == false || maxDiff <= diff)
            {
                maxDiff = diff;
                diff1 = d1;
                diff2 = d2;
                idx = i;
                setFlag = true;
            }
        }
    }

    // allot to node with smaller diff
    if (diff1 < diff2)
    {
        node1->elements[node1->count++] = node->elements[idx];
        node->elements[idx]->container = node1;
    }
    else if (diff1 > diff2)
    {
        node2->elements[node2->count++] = node->elements[idx];
        node->elements[idx]->container = node2;
    }
    // if diff is same, allot to node with the MBR having smaller area
    else if (area1 > area2)
    {
        node2->elements[node2->count++] = node->elements[idx];
        node->elements[idx]->container = node2;
    }
    else if (area2 > area1)
    {
        node1->elements[node1->count++] = node->elements[idx];
        node->elements[idx]->container = node1;
    }
    // if area is also same, then allot to the node with less no of elements
    else if (node1->count > node2->count)
    {
        node2->elements[node2->count++] = node->elements[idx];
        node->elements[idx]->container = node2;
    }
    else
    {
        node1->elements[node1->count++] = node->elements[idx];
        node->elements[idx]->container = node1;
    }
}

// main split function
SplitResult *nodeSplit(Node *node)
{
    // two splitted nodes
    Node *node1 = createNode(NULL, node->isLeaf);
    Node *node2 = createNode(NULL, node->isLeaf);

    pickSeeds(node, node1, node2);

    while (node1->count + node2->count < node->count)
    {
        createNodeParent(node1);
        createNodeParent(node2);

        // node2 is underflowed
        if (MAX_ENTRIES + 1 - node1->count == MIN_ENTRIES)
        {
            for (int i = 0; i < node->count; i++)
            {
                if (!isPresent(node1->elements, node1->count, node->elements[i]) &&
                    !isPresent(node2->elements, node2->count, node->elements[i]))
                {
                    node2->elements[node2->count++] = node->elements[i];
                    node->elements[i]->container = node2;
                }
            }
        }
        // node1 is underflowed
        else if (MAX_ENTRIES + 1 - node2->count == MIN_ENTRIES)
        {
            for (int i = 0; i < node->count; i++)
            {
                if (!isPresent(node1->elements, node1->count, node->elements[i]) &&
                    !isPresent(node2->elements, node2->count, node->elements[i]))
                {
                    node1->elements[node1->count++] = node->elements[i];
                    node->elements[i]->container = node1;
                }
            }
        }
        else
        {
            pickNext(node, node1, node2);
        }
    }

    NodeEle *parent = node->parent;
    free(node);

    SplitResult *split = (SplitResult *)malloc(sizeof(SplitResult));
    split->parent = parent;
    split->leaf1 = node1;
    split->leaf2 = node2;

    return split;
}
/* ADJUST TREE */

// adjustTree function to propagate changes up in subtree
// leaf1 == leaf2 of split is condition if node did not split
SplitResult *adjustTree(SplitResult *split)
{
    // Init local variables
    SplitResult *splitOp = split;
    Node *nodeOp1 = splitOp->leaf1;
    Node *nodeOp2 = splitOp->leaf2;
    NodeEle *parentOp = splitOp->parent;

    while (parentOp != NULL)  // Stop at root node
    {
        // Update parent nodes with apporpriate MBRs
        updateParent(parentOp, nodeOp1, nodeOp2);
        parentOp = nodeOp1->parent;

        // Check if parent needs to be split
        if (parentOp->container->count > MAX_ENTRIES)
        {
            if (splitOp != NULL) free(splitOp);
            splitOp = nodeSplit(parentOp->container);
            nodeOp1 = splitOp->leaf1;
            nodeOp2 = splitOp->leaf2;
            parentOp = splitOp->parent;
        }
        else
        {
            if (splitOp != NULL) free(splitOp);
            splitOp = NULL;  // Assign splitOp to NULL if parent need not split
            nodeOp1 = parentOp->container;
            nodeOp2 = nodeOp1;
            parentOp = nodeOp1->parent;
        }
    }

    if (splitOp == NULL)  // Check if root did not split
    {
        splitOp = (SplitResult *)malloc(sizeof(SplitResult));
        splitOp->parent = parentOp;
        splitOp->leaf1 = nodeOp1;
        splitOp->leaf2 = nodeOp2;
    }

    return splitOp;
}

/* INSERT FUNCTION */

// Insert function incorporating all other files
void insert(Rtree *tree, Point bottomLeft, Point topRight)
{
    Rect mbr = {topRight, bottomLeft};
    // choose leaf based on elem
    Node *leaf = ChooseLeaf(tree, mbr);
    leaf->elements[leaf->count++] =
        createNodeEle(leaf, topRight, bottomLeft);  // create node_ele for element to be added
    SplitResult *split = NULL;

    if (leaf->count > MAX_ENTRIES)  // node overflowed -> node requires splitting
    {
        split = nodeSplit(leaf);
    }
    else
    {
        split = (SplitResult *)malloc(sizeof(SplitResult));
        split->parent = leaf->parent;
        split->leaf1 = leaf;
        split->leaf2 = leaf;
    }
    split = adjustTree(split);
    if (split->leaf1 != split->leaf2)
    {
        Node *root = createNode(NULL, false);
        createNodeParent(split->leaf1);
        createNodeParent(split->leaf2);
        NodeEle *dummy = createNodeEle(root, topRight, bottomLeft);
        root->elements[root->count++] = dummy;
        updateParent(dummy, split->leaf1, split->leaf2);
        tree->root = root;
    }
}

/* -----------------------SEARCH FUNCTION------------------------------------------------- */

// searches for searchRect in searchNode
void search(Node *searchNode, Rect searchRect)
{
    for (int i = 0; i < searchNode->count; i++)  // iterates over the MBRs present in the passed node
    {
        if (searchNode->elements[i] != NULL && isOverlap(searchRect, searchNode->elements[i]->mbr))
        {
            if (searchNode->isLeaf)  // overlapped MBR is part of a leaf node (datapoint)
            {
                if (searchNode->elements[i]->mbr.topRight.x == searchNode->elements[i]->mbr.bottomLeft.x &&
                    searchNode->elements[i]->mbr.topRight.y == searchNode->elements[i]->mbr.bottomLeft.y)
                    printf("Search MBR overlaps with leaf element: (%d, %d)\n", searchNode->elements[i]->mbr.topRight.x,
                           searchNode->elements[i]->mbr.topRight.y);
                else
                    printf("Search MBR overlaps with leaf element: (%d, %d) -> (%d, %d)\n",
                           searchNode->elements[i]->mbr.bottomLeft.x, searchNode->elements[i]->mbr.bottomLeft.y,
                           searchNode->elements[i]->mbr.topRight.x, searchNode->elements[i]->mbr.topRight.y);
            }

            // Descend into tree if node is not leaf
            if (searchNode->elements[i]->child != NULL && searchNode->isLeaf == false)
                search(searchNode->elements[i]->child, searchRect);
        }
    }
}

/* ------------------------MAIN FUNCTION-------------------------------------------------- */

int main()
{
    FILE *fp = fopen("data.txt", "r");

    // Print error message if file opening fails
    if (fp == NULL)
    {
        perror("File opening failed with");
        return 1;
    }

    Rect mbr;
    int x, y;
    Rtree *tree = createRtree();

    // insert all the points
    while (fscanf(fp, "%d %d\n", &x, &y) != EOF)
    {
        Point bottomLeft = {x, y};
        Point topRight = {x, y};
        insert(tree, bottomLeft, topRight);
    }

    fclose(fp);
    traversal(tree->root, true);

    printf("-----------------------------------------\n");
    // This way we can call the search function
    //  Rect searchRect;
    //  searchRect.bottomLeft.x = 1;
    //  searchRect.bottomLeft.y = 8;
    //  searchRect.topRight.x = 12;
    //  searchRect.topRight.y = 20;
    //  search(tree->root, searchRect);

    return 0;
}
