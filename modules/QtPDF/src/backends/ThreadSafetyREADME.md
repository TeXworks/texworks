Policies for Backend Implementations
================================

  1. All non-const data may only be accessed after acquiring at least a read
     lock of the corresponding _docLock or _pageLock. If the data is to be
     changed, a write lock is required. Note that locks cannot be locked
     recursively, but cannot be upgraded.
  2. If a doc-lock and a page-lock are acquired, the doc-lock **must be
     acquired first** to avoid deadlocks.
  3. Classes derived from Page should have a protected constructor (and hence
     the corresponding class derived from Document as friend).
  4. In the constructor of classes derived from Page (and methods called from
     it), don't try to acquire a doc-read lock.
  5. In the destructor of classes derived from Document, clearPages() should be
     called.

Good practice:

  - When methods that need a read-lock are used for internal purposes as well
    as from outside, there should be a protected, internal implementation that
    assumes all locks are properly acquired beforehand as well as a public
    method that acquires the necessary locks and then invokes the internal
    method. This ensures that the internal method can also be used from methods
    that acquire write locks (trying to acquire a read lock in that situation
    would block the thread indefinitely).


Internal Design
=============

The reason for policy 1 should be fairly obvious.

The reason for policy 2 is the following. Suppose we have two threads A and B.
Let's assume that A has acquired a write-lock to the document and B has acquired
a write-lock on a page. Now picture consider the situation that A tries to
acquire a lock on the same page, while B tries to acquire a lock on the
document. Both will block, waiting for the other to release the write lock,
which will never happen.

This is also the reason why the Page class needs a const QSharedPointer to the
corresponding Document's _docLock. If it wouldn't have a constant direct handle
to the _docLock, it would need to lock _pageLock first to access _parent and
from it the _docLock. This would break policy 2, however. Using a QSharedPointer
further ensures that _docLock.data() is not deallocated when the Document goes
out of scope if there are still Pages needing it. This could also be realized by
a QWeakPointer which would have to be upgraded to a QSharedPointer and checked
each time it is used, however.

The reason for policy 3 is that the page classes are only helpers to encapsulate
data. They must not be constructed from any object except the corresponding
Document implementation.

The reason for policy 4 is that pages are only created by a Document::page()
implementation. Since that will add the page to its _pages list it needs to hold
a write-lock to the document. Hence, trying to acquire a read-lock would block
the thread indefinitely.

The reason for policy 5 is that after the derived object is destroyed, no
derived Page object should access it anymore (as implementation-specific data is
no longer available).
