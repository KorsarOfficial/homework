# Educational Platform — C++ Implementation

## Overview

A C++17 rewrite of a Django REST Framework educational platform. The original project uses Django 5.1.7 + DRF + Celery + Redis + PostgreSQL + Stripe to manage courses, lessons, and subscriptions. This implementation models the same domain entirely in-memory using STL containers (`std::map`, `std::vector`, `std::unordered_map`) with algorithmic complexity guarantees on every operation.

The program provides an interactive console menu for browsing courses, lessons, managing subscriptions, generating reports, and checking ownership/subscription permissions.

## Mathematical Formalization

### Data Structures

- **Users**: `U = {u₁, u₂, ..., uₙ}` where `uᵢ = (id, username, email, is_moderator)`
- **Courses**: `C : id → Course`, stored in `std::map` for O(log |C|) lookup
- **Lessons**: `L : id → Lesson`, stored in `std::map` for O(log |L|) lookup
- **Subscriptions**: `S ⊆ U.id × C.id`, stored in `std::vector`

### Key Operations

**Subscription Toggle**

```
S' = S △ {(u, c)}
```

Where `△` denotes symmetric difference. If `(u, c) ∈ S`, then `S' = S \ {(u, c)}`; otherwise `S' = S ∪ {(u, c)}`. Time complexity: **O(|S|)** for linear scan.

**Courses by Lesson Count**

```
sort C by |{l ∈ L : l.course_id = c.id}| descending
```

First compute lesson counts per course in O(|L|) using an unordered map, then `stable_sort` the result. Total: **O(|L| + |C| log |C|)**.

**Count Subscribers per Course**

```
f(c) = |{s ∈ S : s.course_id = c}|   ∀ c ∈ C
```

Single pass over `S` with hash map accumulation. Time complexity: **O(|S|)**.

**Courses for User (Permission Filter)**

```
F(C, u) = { c ∈ C | owner(c) = u ∨ is_moderator(u) }
```

Moderators see all courses; regular users see only owned courses. Time complexity: **O(|C|)**.

**Ownership Check**

```
owner(u, c) ↔ ∃c ∈ C : C[c].owner_id = u
```

Map lookup: **O(log |C|)**.

**Subscription Check**

```
has_sub(u, c) ↔ (u, c) ∈ S
```

Linear scan: **O(|S|)**.

## Original Code Analysis

Issues found in the Django project:

### 1. `TextField` with `max_length` — Semantic Mismatch

The models use `TextField` with a `max_length` parameter. In Django, `TextField` is intended for unbounded text and the `max_length` argument only affects form validation, not the database column. The correct field for length-bounded text is `CharField`. This is a common misunderstanding that leads to a mismatch between declared intent and actual database behavior (PostgreSQL `text` type ignores length constraints).

### 2. No Application-Level Duplicate Subscription Prevention

Subscription uniqueness relies solely on `unique_together` in the model's `Meta` class. There is no application-level check before attempting to create a subscription. This means the only protection is a database `IntegrityError` on duplicates, which results in an unhandled 500 error rather than a clean API response. The toggle logic should check existence first or catch the exception gracefully.

### 3. `__str__` Indented Inside `Meta` Class — Dead Code

In at least one model, the `__str__` method is indented inside the `Meta` inner class. Python does not raise an error because `Meta` is just a regular class that happens to be read by Django's metaclass machinery — unknown attributes and methods are silently ignored. The `__str__` method inside `Meta` never gets called as a string representation of the outer model. It is effectively dead code.

### 4. Celery Task with Synchronous SMTP — Defeats Async Purpose

The `send_update_mail` Celery task calls Django's `send_mail()` which performs a synchronous SMTP connection. If the SMTP server is slow or unresponsive, the Celery worker thread blocks, negating the benefit of offloading the task to an async queue. The task should use a non-blocking email backend or at minimum set appropriate timeouts.

### 5. No Pagination on Lesson List View

`LessonListAPIView` returns all lessons in the database without any pagination. For a growing platform, this means unbounded response sizes, increasing memory usage, and degrading response times. DRF pagination classes (`PageNumberPagination`, `LimitOffsetPagination`) should be configured either globally or on the view.

### 6. AI Code Markers — Repetitive Boilerplate Patterns

The codebase exhibits signs of AI-generated code: identical ViewSet patterns copy-pasted across apps, generic variable names (`obj`) used everywhere without domain-specific naming, and serializers that are pure boilerplate with no custom validation, computed fields, or business logic. The uniformity suggests generation rather than intentional design.

## Build

```bash
g++ -std=c++17 -O2 -o homework main.cpp
```

Requires a C++17-compatible compiler (GCC 7+, Clang 5+, MSVC 19.14+).

### Run

```bash
./homework
```

The program starts with preloaded sample data (3 users, 5 courses, 12 lessons, 5 subscriptions) and presents an interactive menu.
