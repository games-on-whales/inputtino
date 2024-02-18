<script lang="ts">

    import {request, selected_device} from "./stores";
    import {onDestroy} from "svelte";

    let canvas: HTMLCanvasElement;

    async function lock_mouse() {
        await canvas.requestPointerLock({
            unadjustedMovement: true,
        });
    }

    document.addEventListener("pointerlockchange", lockChangeAlert, false);

    function lockChangeAlert() {
        if (document.pointerLockElement === canvas) {
            console.log("The pointer lock status is now locked");
            document.addEventListener("mousemove", updatePosition, false);
        } else {
            console.log("The pointer lock status is now unlocked");
            document.removeEventListener("mousemove", updatePosition, false);
        }
    }

    onDestroy(() => {
        document.removeEventListener("mousemove", updatePosition, false);
    });

    function updatePosition(e: MouseEvent) {
        if ($selected_device?.type === "MOUSE") {
            request("/devices/mouse/" + $selected_device?.device_id + "/move_rel", "POST", {
                "delta_x": e.movementX,
                "delta_y": e.movementY
            });
        }
    }
</script>

<p class="text-center dark:text-white">Click to lock the mouse</p>

<canvas width="640" height="360" bind:this={canvas} on:click={lock_mouse}>
    <p>Your browser does not support the canvas element.</p>
</canvas>

<style>
    canvas {
        display: block;
        margin: 0 auto;
        border: 1px solid black;
    }
</style>