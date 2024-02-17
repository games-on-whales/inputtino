<script lang="ts">
    import {type DeviceData, available_devices, selected_device, request} from "./stores";
    import {ToolbarButton, Select} from "flowbite-svelte";
    import {RefreshOutline} from "flowbite-svelte-icons";

    async function fetch_devices(): Promise<DeviceData[]> {
        const data = await request('/devices');
        return data?.devices;
    }

    async function refresh_devices() {
        const devices = await fetch_devices().catch(_error => {
            return [];
        });
        available_devices.set(devices);
    }
</script>

<ToolbarButton outline={true} on:click={refresh_devices}>
    <RefreshOutline></RefreshOutline>
</ToolbarButton>

<div>
    <Select size="lg" placeholder="Select a device" bind:value={$selected_device}>
        <option value={null} disabled selected hidden>Select a device</option>
        {#each $available_devices as device (device.device_id)}
            <option value="{device}">
                {device.type}
            </option>
        {/each}
    </Select>
</div>