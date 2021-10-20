Bugfixes, improvements, etc are welcome! Part of code reviews will be checking
consistency with Clickitongue's style. That style is basically Google C++ style,
with the following changes:

* Use symmetric curly braces:

```
void example()
{
  if (hello)
  {
    foo();
    bar();
  }
}
```

* Do aim for 80 characters per row, but it's ok to go a few over.
* Skip curly braces on ifs and loops if the condition fits on one line, and the
  body either fits on one line, or is a loop/if that is itself skipping braces.
  Basically:

```
if (foo)
  bar();

for (auto const& x : stuff)
  if (whatever)
    for (auto const& y : other_stuff)
      doSomething();

while (condition)
{
  if (some_interesting_condition == a_thing && other_condition == thing2 &&
      now_we_are_on_the_next_line == yes_we_are)
  {
    writeTheBraces();
  }
}
```

* Write functions `firstWordUncapitalized()` rather than `FirstWordCapitalized()`.
* Use `Type const& foo` rather than `const Type& foo`.
* Use C-style casts for numeric casts like int->float. (Do use appropriate C++ ones for all other casts).
* Align public/private/protected with the `class` keyword, rather than indented one space.
* When a member function definition is too long, wrap after the `::`
  class identifier. Example:

```
SortOfLongReturnType FairlyLongClassName::
longMemberFunctionName(SomeType const& some_type, int a_number_with_a_long_name,
                       double another_number) const
{
```
