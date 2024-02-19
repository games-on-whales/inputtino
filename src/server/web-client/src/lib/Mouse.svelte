<script lang="ts">

    import {request, selected_device} from "./stores";
    import {onDestroy, onMount} from "svelte";

    let element: HTMLElement;

    async function lock_mouse() {
        await element.requestPointerLock({
            unadjustedMovement: true,
        });
    }

    function lockChangeAlert() {
        if (document.pointerLockElement === element) {
            document.addEventListener("mousemove", updatePosition, false);
            document.addEventListener("mousedown", updateButtons, false);
            document.addEventListener("mouseup", updateButtons, false);
            document.addEventListener("wheel", updateWheel, false);
        } else {
            document.removeEventListener("mousemove", updatePosition, false);
            document.removeEventListener("mousedown", updateButtons, false);
            document.removeEventListener("mouseup", updateButtons, false);
            document.removeEventListener("wheel", updateWheel, false);
        }
    }

    onMount(() => {
        document.addEventListener("pointerlockchange", lockChangeAlert, false);
    });

    onDestroy(() => {
        document.removeEventListener("mousemove", updatePosition, false);
        document.removeEventListener("mousedown", updateButtons, false);
        document.removeEventListener("mouseup", updateButtons, false);
        document.removeEventListener("wheel", updateWheel, false);
        document.removeEventListener("pointerlockchange", lockChangeAlert, false);
    });

    function updateWheel(e: WheelEvent) {
        if ($selected_device?.type === "MOUSE") {
            let scale = 1.0;
            if (e.deltaMode === WheelEvent.DOM_DELTA_LINE) {
                // The delta* values are specified in lines. Each mouse click scrolls a line of content
                scale = 10.0;
            } else if (e.deltaMode === WheelEvent.DOM_DELTA_PAGE) {
                // The delta* values are specified in pages. Each mouse click scrolls a page of content
                scale = 100.0;
            }
            const vertical = e.deltaY != 0;
            request("/devices/mouse/" + $selected_device?.device_id + "/scroll", "POST", {
                "direction": vertical ? "VERTICAL" : "HORIZONTAL",
                "distance": vertical ? e.deltaY * scale : e.deltaX * scale
            })
        }
    }

    function updatePosition(e: MouseEvent) {
        if ($selected_device?.type === "MOUSE") {
            request("/devices/mouse/" + $selected_device?.device_id + "/move_rel", "POST", {
                "delta_x": e.movementX,
                "delta_y": e.movementY
            });
        }
    }

    const btn_type: Record<number, string> = {
        0: "LEFT",
        1: "MIDDLE",
        2: "RIGHT",
        3: "SIDE",
        4: "EXTRA"
    }

    function updateButtons(e: MouseEvent) {
        if ($selected_device?.type === "MOUSE") {
            if (e.type === "mousedown") {
                request("/devices/mouse/" + $selected_device?.device_id + "/press", "POST", {
                    "button": btn_type[e.button]
                })
            } else if (e.type === "mouseup") {
                request("/devices/mouse/" + $selected_device?.device_id + "/release", "POST", {
                    "button": btn_type[e.button]
                })
            }
        }
    }
</script>

<p class="text-center dark:text-white">Click to lock the mouse</p>

<canvas width="640" height="360"
        bind:this={element}
        on:click={lock_mouse}
        class="rounded-b-3xl border-2 border-purple-700 dark:border-purple-400">
    <p>Your browser does not support the canvas element.</p>
</canvas>

<style>
    canvas {
        display: block;
        margin: 0 auto;
    }
</style>