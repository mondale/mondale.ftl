# mondale.ftl

`mondale.ftl`

## Style Guidance

The following style guidance should be used for all code. Examples are
considered to be authoritative.

### Naming.

```
int global_variable_is_prefixed_by_global = 0;

void MethodDeclaration();

class ClassDeclaration final {
 public:
  ...
 private:
  ...
};

```

### Exceptions.

Fils may be individually labeled as exempt from this guidance. This is commonly
used for AI-implemented sections of the codebase. An exemption takes the form of

`// Exempt from style expectations.`

