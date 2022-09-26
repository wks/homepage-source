using System;
using System.Collections.Generic;

interface NestedList
{
    IEnumerable<int> Traverse();
}

class Leaf : NestedList
{
    private int num;

    public Leaf(int num)
    {
        this.num = num;
    }

    public IEnumerable<int> Traverse()
    {
        yield return num;
    }
}

class Branch : NestedList
{
    private List<NestedList> children;

    public Branch()
    {
        children = new List<NestedList>();
    }

    public Branch Add(NestedList child)
    {
        children.Add(child);
        return this;
    }

    public IEnumerable<int> Traverse()
    {
        foreach (var child in children)
        {
            // It is stackless.  We need to yield from the inner iterators.
            foreach (var y in child.Traverse())
            {
                yield return y;
            }
        }
    }
}

public class IteratorTraversal
{
    public static void Main()
    {
        // [1, [[2, 3], [4, 5]], [6, 7, 8]]
        var nestedList = new Branch()
            .Add(new Leaf(1))
            .Add(new Branch()
                    .Add(new Branch()
                        .Add(new Leaf(2))
                        .Add(new Leaf(3)))
                    .Add(new Branch()
                        .Add(new Leaf(4))
                        .Add(new Leaf(5))))
            .Add(new Branch()
                    .Add(new Leaf(6))
                    .Add(new Leaf(7))
                    .Add(new Leaf(8)));

        foreach (var n in nestedList.Traverse()) // Create the iterator
        {
            Console.WriteLine(n);
        }
    }
}
