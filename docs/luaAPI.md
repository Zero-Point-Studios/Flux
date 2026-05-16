# Flux documentation - Lua API

 Welcome to the manual of scripting in Flux, this will be your scripting reference manual (if you don't know what you're doing). You can use these built-in functions to control your game!

---

## Global functions

### **onStart()**

* This is called when the game has started. (it's pretty simple and self-explanatory,)
##### This is pretty useful for loading by the way.

Example:
```lua
function onStart()
	print("Hello World!")
	print("Loading data...")
	Load()
	print("Load Successfully!")
end
```
### **onUpdate()**

* This function is called on every frame of the game, pretty useful if you can handle it right.

Example:
```lua
local count = 0
function onUpdate()
	if count > 10 then
		count = count + 1
		print(tostring(count))
	end
end
```

### **onEnd()**

* This function is called right before the game ends
##### psst, it's pretty useful if you want to make a saving system ;)

```lua
function onEnd()
	print("Saving...")
	Save()
	print("Saved successfully!")
end
```

---
## Input system
The input system usually starts with a `Input.` syntax.

## Keyboard inputs


### **Input.isKeyDown(keyName)**
Checks if a specific keyboard key is being held down.
* **Arguments:** `keyName` (string) - The name of the key (example: `"W"`, `"Space"`, `"Escape"`).
* **Returns:** `bool` - It  returns `true` if the key is pressed, otherwise it'll return `false`

Example:
``` lua
function onUpdate()
	if Input.isKeyDown("W") then
		obj:setPosition(px, py, pz + 0.1) -- Moves forward when the W key is pressed
	end
end
```

### **Input.anyKey()**
Checks if a specific keyboard key is being held down.
* **Returns:** `bool` - Returns `true` when even a single key is pressed, otherwise it'll return `false`

Example:
``` lua
function onUpdate()
	if Input.anyKey() then
        print("A key is being pressed.")
    end
end
```

### **Input.getKeyPressed()**
Checks what keyboard key is being held down.
* **Returns:** `bool` - Returns `true` when even a single key is pressed, otherwise it'll return `false`

Example:
``` lua
function onUpdate()
	if Input.anyKey() then
        print("A key is being pressed.")
    end
end
```
---

### Mouse Inputs

### **Input.getMouseX()**

Gets the current horizontal (or X) coordinate of the mouse cursor.

* **Returns:** `float` - The X position of the mouse cursor. (pretty basic)

Example:
```lua
function onUpdate()
    local mouseX = Input.getMouseX()
    print("Mouse X: " .. tostring(mouseX))
end
```

### **Input.getMouseY()**

Gets the current vertical (or Y) coordinate of the mouse cursor.

* **Returns:** `float` - The Y position of the mouse cursor. (just like `Input.getMouseX()` but Y instead of X)

Example:
```lua
function onUpdate()
    local mouseY = Input.getMouseY()
    print("Mouse Y: " .. tostring(mouseY))
end
```

### **Input.isMouseDown(mouseButtonIndex)**
Checks if a specific mouse button (`mouseButtonIndex`) is being held down.

* **Arguments:** `mouseButtonIndex` (integer) - The index of the mouse button (example: 1 for Left Click, 2 for Middle Click, 3 for Right Click).

```lua
function onUpdate()
    if Input.isMouseDown(1) then
        print("Left mouse button is clicked!")
    end
end
```

## Engine API
The engine API lets you manipulate scene objects, manage game states and more that controls the engine just by using the `Engine.` prefix.

### **Vector3**