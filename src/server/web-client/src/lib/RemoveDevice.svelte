<script lang="ts">
    import {type DeviceData, type JsonError, available_devices, selected_device, request} from "./stores";
    import {ToolbarButton} from "flowbite-svelte";
    import {MinusSolid} from "flowbite-svelte-icons";

    async function remove_device(device: DeviceData) {
        const response = await request('/devices/' + device.device_id, "DELETE");
        // Remove the device from the local cache and reset the selected device
        available_devices.update(devices => {
            const remaining_devices = devices.filter(d => d.device_id !== device.device_id);
            selected_device.set(remaining_devices.length > 0 ? remaining_devices[0] : null);
            return remaining_devices;
        });
    }
</script>

<ToolbarButton outline on:click={() => $selected_device ? remove_device($selected_device) : null}>
    <MinusSolid></MinusSolid>
</ToolbarButton>