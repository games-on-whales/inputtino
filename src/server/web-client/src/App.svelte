<script lang="ts">
    // import svelteLogo from './assets/svelte.svg'
    // import viteLogo from '/vite.svg'
    import Devices from './lib/Devices.svelte'
    import AddDevice from "./lib/AddDevice.svelte";
    import RemoveDevice from "./lib/RemoveDevice.svelte";
    import FetchRequest from "./lib/FetchRequest.svelte";
    import {Toolbar, ToolbarGroup, DarkMode, Toast} from 'flowbite-svelte';
    import {FireOutline} from "flowbite-svelte-icons";
</script>

<main>
    <Toolbar class="bg-slate-200 dark:bg-gray-700">
        <AddDevice/>
        <RemoveDevice/>
        <Devices/>

        <DarkMode slot="end"/>
    </Toolbar>

    <FetchRequest />
</main>

<style>
</style>
