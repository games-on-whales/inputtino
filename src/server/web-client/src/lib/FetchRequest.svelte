<script lang="ts">

    import {Toast} from "flowbite-svelte";
    import {FireOutline} from "flowbite-svelte-icons";
    import {fetch_error} from "./stores";
    import {fly} from "svelte/transition";
</script>

{#each $fetch_error as error, index}
    <Toast role="alert" color="red"
           class="absolute right-5 bg-red-600 text-white dark:bg-red-100 dark:text-red-600"
           style="bottom: {index * 90}px;"
           transition={fly}
           on:close={() => fetch_error.update(errors => errors.toSpliced(index,1))}>
        <FireOutline slot="icon"/>
        {error.message}
    </Toast>
{/each}