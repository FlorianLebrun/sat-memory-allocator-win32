
enum BtKeyStatus
{
  Duplicate,
  SearchFailure,
  Success,
  InsertIt,
  LessKeys
};

template <typename BtKey, int M = 3>
struct Btree
{
protected:
  struct BtNode
  {
    int n;             /* n < M No. of keys in node will always less than order of B tree */
    BtKey keys[M - 1]; /*array of keys*/
    BtNode *p[M];      /* (n+1 pointers will be in use) */
  };

private:
  BtNode *root;
  BtKeyStatus ins(BtNode *r, BtKey x, BtKey *y, BtNode **u);
  BtKeyStatus del(BtNode *r, BtKey x);
  BtKeyStatus Btree<BtKey, M>::delUpper(BtNode *ptr, BtKey key, BtKey& fkey);
  int searchPos(BtKey x, BtKey *key_arr, int n);
  void displayNode(BtNode *root, int blanks);
  virtual BtNode* allocNode() = 0;
  virtual void freeNode(BtNode* node) = 0;

public:
  Btree();
  bool contains(BtKey key);
  bool insert(BtKey key);
  bool remove(BtKey key);
  bool Btree<BtKey, M>::removeUpper(BtKey key, BtKey& found);
  void display();
};

template <typename BtKey, int M>
Btree<BtKey, M>::Btree()
{
  this->root = 0;
}

template <typename BtKey, int M>
bool Btree<BtKey, M>::insert(BtKey key)
{
  BtNode *newnode;
  BtKey upKey;
  BtKeyStatus value;
  value = ins(root, key, &upKey, &newnode);
  if (value == Duplicate)
  {
    return false;
  }
  if (value == InsertIt)
  {
    BtNode *uproot = root;
    root = this->allocNode();
    root->n = 1;
    root->keys[0] = upKey;
    root->p[0] = uproot;
    root->p[1] = newnode;
  }
  return true;
}

template <typename BtKey, int M>
BtKeyStatus Btree<BtKey, M>::ins(BtNode *ptr, BtKey key, BtKey *upKey, BtNode **newnode)
{
  BtNode *newPtr, *lastPtr;
  int pos, i, n, splitPos;
  BtKey newKey, lastKey;
  BtKeyStatus value;
  if (ptr == 0)
  {
    *newnode = 0;
    *upKey = key;
    return InsertIt;
  }
  n = ptr->n;
  pos = searchPos(key, ptr->keys, n);
  if (pos < n && key == ptr->keys[pos])
    return Duplicate;
  value = ins(ptr->p[pos], key, &newKey, &newPtr);
  if (value != InsertIt)
    return value;
  /*If keys in node is less than M-1 where M is order of B tree*/
  if (n < M - 1)
  {
    pos = searchPos(newKey, ptr->keys, n);
    /*Shifting the key and pointer right for inserting the new key*/
    for (i = n; i > pos; i--)
    {
      ptr->keys[i] = ptr->keys[i - 1];
      ptr->p[i + 1] = ptr->p[i];
    }
    /*Key is inserted at exact location*/
    ptr->keys[pos] = newKey;
    ptr->p[pos + 1] = newPtr;
    ++ptr->n; /*incrementing the number of keys in node*/
    return Success;
  } /*End of if */
  /*If keys in nodes are maximum and position of node to be inserted is last*/
  if (pos == M - 1)
  {
    lastKey = newKey;
    lastPtr = newPtr;
  }
  else /*If keys in node are maximum and position of node to be inserted is not last*/
  {
    lastKey = ptr->keys[M - 2];
    lastPtr = ptr->p[M - 1];
    for (i = M - 2; i > pos; i--)
    {
      ptr->keys[i] = ptr->keys[i - 1];
      ptr->p[i + 1] = ptr->p[i];
    }
    ptr->keys[pos] = newKey;
    ptr->p[pos + 1] = newPtr;
  }
  splitPos = (M - 1) / 2;
  (*upKey) = ptr->keys[splitPos];

  (*newnode) = this->allocNode(); /*Right node after split*/
  ptr->n = splitPos;                             /*No. of keys for left splitted node*/
  (*newnode)->n = M - 1 - splitPos;              /*No. of keys for right splitted node*/
  for (i = 0; i < (*newnode)->n; i++)
  {
    (*newnode)->p[i] = ptr->p[i + splitPos + 1];
    if (i < (*newnode)->n - 1)
      (*newnode)->keys[i] = ptr->keys[i + splitPos + 1];
    else
      (*newnode)->keys[i] = lastKey;
  }
  (*newnode)->p[(*newnode)->n] = lastPtr;
  return InsertIt;
}

template <typename BtKey, int M>
bool Btree<BtKey, M>::contains(BtKey key)
{
  int pos, n;
  BtNode *ptr = root;
  while (ptr)
  {
    n = ptr->n;
    pos = searchPos(key, ptr->keys, n);
    if (pos < n && key == ptr->keys[pos])
    {
      return true;
    }
    ptr = ptr->p[pos];
  }
  return false;
}

template <typename BtKey, int M>
int Btree<BtKey, M>::searchPos(BtKey key, BtKey *key_arr, int n)
{
  int pos = 0;
  while (pos < n && key > key_arr[pos])
    pos++;
  return pos;
}

template <typename BtKey, int M>
bool Btree<BtKey, M>::remove(BtKey key)
{
  BtNode *uproot;
  BtKeyStatus value;
  value = del(root, key);
  switch (value)
  {
  case SearchFailure:
    return false;
  case LessKeys:
    uproot = root;
    root = root->p[0];
    this->freeNode(uproot);
  }
  return true;
}

template <typename BtKey, int M>
BtKeyStatus Btree<BtKey, M>::del(BtNode *ptr, BtKey key)
{
  int pos, i, pivot, n, min;
  BtKey *key_arr;
  BtKeyStatus value;
  BtNode **p, *lptr, *rptr;

  if (ptr == 0)
    return SearchFailure;
  /*Assigns values of node*/
  n = ptr->n;
  key_arr = ptr->keys;
  p = ptr->p;
  min = (M - 1) / 2; /*Minimum number of keys*/

  //Search for key to delete
  pos = searchPos(key, key_arr, n);
  // p is a leaf
  if (p[0] == 0)
  {
    if (pos == n || key < key_arr[pos])
      return SearchFailure;
    /*Shift keys and pointers left*/
    for (i = pos + 1; i < n; i++)
    {
      key_arr[i - 1] = key_arr[i];
      p[i] = p[i + 1];
    }
    return --ptr->n >= (ptr == root ? 1 : min) ? Success : LessKeys;
  }

  //if found key but p is not a leaf
  if (pos < n && key == key_arr[pos])
  {
    BtNode *qp = p[pos], *qp1;
    int nkey;
    for (;;)
    {
      nkey = qp->n;
      qp1 = qp->p[nkey];
      if (qp1 == 0)
        break;
      qp = qp1;
    }
    key_arr[pos] = qp->keys[nkey - 1];
    qp->keys[nkey - 1] = key;
  }
  value = del(p[pos], key);
  if (value != LessKeys)
    return value;

  if (pos > 0 && p[pos - 1]->n > min)
  {
    pivot = pos - 1; /*pivot for left and right node*/
    lptr = p[pivot];
    rptr = p[pos];
    /*Assigns values for right node*/
    rptr->p[rptr->n + 1] = rptr->p[rptr->n];
    for (i = rptr->n; i > 0; i--)
    {
      rptr->keys[i] = rptr->keys[i - 1];
      rptr->p[i] = rptr->p[i - 1];
    }
    rptr->n++;
    rptr->keys[0] = key_arr[pivot];
    rptr->p[0] = lptr->p[lptr->n];
    key_arr[pivot] = lptr->keys[--lptr->n];
    return Success;
  }
  //if (posn > min)
  if (pos < n && p[pos + 1]->n > min)
  {
    pivot = pos; /*pivot for left and right node*/
    lptr = p[pivot];
    rptr = p[pivot + 1];
    /*Assigns values for left node*/
    lptr->keys[lptr->n] = key_arr[pivot];
    lptr->p[lptr->n + 1] = rptr->p[0];
    key_arr[pivot] = rptr->keys[0];
    lptr->n++;
    rptr->n--;
    for (i = 0; i < rptr->n; i++)
    {
      rptr->keys[i] = rptr->keys[i + 1];
      rptr->p[i] = rptr->p[i + 1];
    } /*End of for*/
    rptr->p[rptr->n] = rptr->p[rptr->n + 1];
    return Success;
  }

  if (pos == n)
    pivot = pos - 1;
  else
    pivot = pos;

  lptr = p[pivot];
  rptr = p[pivot + 1];
  /*merge right node with left node*/
  lptr->keys[lptr->n] = key_arr[pivot];
  lptr->p[lptr->n + 1] = rptr->p[0];
  for (i = 0; i < rptr->n; i++)
  {
    lptr->keys[lptr->n + 1 + i] = rptr->keys[i];
    lptr->p[lptr->n + 2 + i] = rptr->p[i + 1];
  }
  lptr->n = lptr->n + rptr->n + 1;
  this->freeNode(rptr); /*Remove right node*/
  for (i = pos + 1; i < n; i++)
  {
    key_arr[i - 1] = key_arr[i];
    p[i] = p[i + 1];
  }
  return --ptr->n >= (ptr == root ? 1 : min) ? Success : LessKeys;
}

template <typename BtKey, int M>
void Btree<BtKey, M>::display()
{
  this->displayNode(this->root, 3);
}

template <typename BtKey, int M>
void Btree<BtKey, M>::displayNode(BtNode *ptr, int blanks)
{
  if (ptr)
  {
    for (int i = 0; i < ptr->n; i++)
    {
      displayNode(ptr->p[i], blanks + 3);
      for (int k = 1; k <= blanks; k++)
        std::cout << ' ';
      uint32_t* parts = (uint32_t*)&ptr->keys[i];
      std::cout << (parts[0]&0x7fffffff)<<":"<< parts[1] << '\n';
    }
    displayNode(ptr->p[ptr->n], blanks + 3);
  }
}

template <typename BtKey, int M>
bool Btree<BtKey, M>::removeUpper(BtKey key, BtKey& found)
{
  BtNode *uproot;
  BtKeyStatus value;
  value = delUpper(root, key, found);
  switch (value)
  {
  case SearchFailure:
    return false;
  case LessKeys:
    uproot = root;
    root = root->p[0];
    this->freeNode(uproot);
  }
  return true;
}

template <typename BtKey, int M>
BtKeyStatus Btree<BtKey, M>::delUpper(BtNode *ptr, BtKey key, BtKey& fkey)
{
  int pos, i, pivot, n, min;
  BtKey *key_arr;
  BtKeyStatus value;
  BtNode **p, *lptr, *rptr;

  if (ptr == 0)
    return SearchFailure;
  /*Assigns values of node*/
  n = ptr->n;
  key_arr = ptr->keys;
  p = ptr->p;
  min = (M - 1) / 2; /*Minimum number of keys*/

                     //Search for upper key to delete
  pos = searchPos(key, key_arr, n);

  // p is a leaf
  if (p[0] == 0)
  {
    if (pos == n)
      return SearchFailure;

    /* use upper key */
    if(key < key_arr[pos]) {
      assert(key <= key_arr[pos]);
      key = key_arr[pos];
    }
    fkey = key;

    /*Shift keys and pointers left*/
    for (i = pos + 1; i < n; i++)
    {
      key_arr[i - 1] = key_arr[i];
      p[i] = p[i + 1];
    }
    return --ptr->n >= (ptr == root ? 1 : min) ? Success : LessKeys;
  }

  //if found key but p is not a leaf
  if (pos < n && key == key_arr[pos])
  {
    BtNode *qp = p[pos], *qp1;
    int nkey;
    fkey = key;
    for (;;)
    {
      nkey = qp->n;
      qp1 = qp->p[nkey];
      if (qp1 == 0)
        break;
      qp = qp1;
    }
    key_arr[pos] = qp->keys[nkey - 1];
    qp->keys[nkey - 1] = key;
  }
  value = delUpper(p[pos], key, fkey);

  // retry with local upper key
  if (value == SearchFailure && key < key_arr[pos]) {
    assert(key <= key_arr[pos]);
    fkey = key = key_arr[pos];
    if (pos < n)
    {
      BtNode *qp = p[pos], *qp1;
      int nkey;
      for (;;)
      {
        nkey = qp->n;
        qp1 = qp->p[nkey];
        if (qp1 == 0)
          break;
        qp = qp1;
      }
      key_arr[pos] = qp->keys[nkey - 1];
      qp->keys[nkey - 1] = key;
    }
    value = delUpper(p[pos], key, fkey);
  }

  if (value != LessKeys)
    return value;

  if (pos > 0 && p[pos - 1]->n > min)
  {
    pivot = pos - 1; /*pivot for left and right node*/
    lptr = p[pivot];
    rptr = p[pos];
    /*Assigns values for right node*/
    rptr->p[rptr->n + 1] = rptr->p[rptr->n];
    for (i = rptr->n; i > 0; i--)
    {
      rptr->keys[i] = rptr->keys[i - 1];
      rptr->p[i] = rptr->p[i - 1];
    }
    rptr->n++;
    rptr->keys[0] = key_arr[pivot];
    rptr->p[0] = lptr->p[lptr->n];
    key_arr[pivot] = lptr->keys[--lptr->n];
    return Success;
  }
  //if (posn > min)
  if (pos < n && p[pos + 1]->n > min)
  {
    pivot = pos; /*pivot for left and right node*/
    lptr = p[pivot];
    rptr = p[pivot + 1];
    /*Assigns values for left node*/
    lptr->keys[lptr->n] = key_arr[pivot];
    lptr->p[lptr->n + 1] = rptr->p[0];
    key_arr[pivot] = rptr->keys[0];
    lptr->n++;
    rptr->n--;
    for (i = 0; i < rptr->n; i++)
    {
      rptr->keys[i] = rptr->keys[i + 1];
      rptr->p[i] = rptr->p[i + 1];
    } /*End of for*/
    rptr->p[rptr->n] = rptr->p[rptr->n + 1];
    return Success;
  }

  if (pos == n)
    pivot = pos - 1;
  else
    pivot = pos;

  lptr = p[pivot];
  rptr = p[pivot + 1];
  /*merge right node with left node*/
  lptr->keys[lptr->n] = key_arr[pivot];
  lptr->p[lptr->n + 1] = rptr->p[0];
  for (i = 0; i < rptr->n; i++)
  {
    lptr->keys[lptr->n + 1 + i] = rptr->keys[i];
    lptr->p[lptr->n + 2 + i] = rptr->p[i + 1];
  }
  lptr->n = lptr->n + rptr->n + 1;
  this->freeNode(rptr); /*Remove right node*/
  for (i = pos + 1; i < n; i++)
  {
    key_arr[i - 1] = key_arr[i];
    p[i] = p[i + 1];
  }
  return --ptr->n >= (ptr == root ? 1 : min) ? Success : LessKeys;
}
